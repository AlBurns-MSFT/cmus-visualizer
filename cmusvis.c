#define _XOPEN_SOURCE_EXTENDED
#include <alloca.h>
#include <locale.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <termios.h>
#include <math.h>
#include <fcntl.h> 

#include <sys/ioctl.h>
#include <fftw3.h>
#define max(a,b) \
	 ({ __typeof__ (a) _a = (a); \
			 __typeof__ (b) _b = (b); \
		 _a > _b ? _a : _b; })
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>
#include <dirent.h>
#include "output/terminal_ncurses.h"
#include "output/terminal_ncurses.c"
#include "input/pulse.h"
#include "input/pulse.c"

#ifdef __GNUC__
// curses.h or other sources may already define
#undef  GCC_UNUSED
#define GCC_UNUSED __attribute__((unused))
#else
#define GCC_UNUSED /* nothing */
#endif

//definitions
int rc;
char *barcolor, *bkcolor;
double gravity, smh, sens;
int bw, bs, autosens;
double smoothDef[64] = {	0.8, 0.8, 1, 1, 0.8, 0.8, 1, 0.8, 0.8, 1, 1, 0.8,
							1, 1, 0.8, 0.6, 0.6, 0.7, 0.8, 0.8, 0.8, 0.8, 0.8,
							0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 
							0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 
							0.8, 0.8, 0.7, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 
							0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6};
double *smooth = smoothDef;
int smcount = 64;
struct audio_data audio;
int col = 6; //color
int bgcol = -1; //background color
int bars = 25;
int M = 2048;
double integral = 0.7;
int framerate = 60;
unsigned int lowcf = 50;
unsigned int highcf = 10000;

//flag for reloading config
int should_reload = 0;

// general: cleanup
void cleanup(void)
{
	cleanup_terminal_ncurses();
}

// general: handle signals
void sig_handler(int sig_no)
{
	if (sig_no == SIGUSR1) {
		should_reload = 1;
		return;
	}
	cleanup();
	if (sig_no == SIGINT) {
		printf("CTRL-C pressed -- goodbye\n");
	}
	signal(sig_no, SIG_DFL);
	raise(sig_no);
}

void load_config(char configPath[255]) 
{	
FILE *fp;
	//config: creating path to default config file
	if (configPath[0] == '\0') {
		char *configFile = "config";
		char *configHome = getenv("XDG_CONFIG_HOME");
		if (configHome != NULL) {
			sprintf(configPath,"%s/%s/", configHome, PACKAGE);
		} else {
			configHome = getenv("HOME");
			if (configHome != NULL) {
				sprintf(configPath,"%s/%s/%s/", configHome, ".config", PACKAGE);
			} else {
				printf("No HOME found (ERR_HOMELESS), exiting...");
				exit(EXIT_FAILURE);
			}
		}
	
		// config: create directory
		mkdir(configPath, 0777);

		// config: adding default filename file
		strcat(configPath, configFile);
		
		fp = fopen(configPath, "ab+");
		if (fp) {
			fclose(fp);
		} else {
			printf("Unable to access config '%s', exiting...\n", configPath);
			exit(EXIT_FAILURE);
		}
	}
	 else { //opening specified file
		fp = fopen(configPath, "rb+");	
		if (fp) {
			fclose(fp);
		} else {
			printf("Unable to open file '%s', exiting...\n", configPath);
			exit(EXIT_FAILURE);
		}
	}


	FILE* fp2 = fopen("new_config", "r+");

	char curChar;
	//advance a line
	while( (curChar = getc(fp2) ) != '\n') {}

	char *sensString = (char*) malloc(10);
	int p;
	for(p = 0; ((curChar = getc(fp2)) != '#'); p++) {
		sensString[p] = curChar;
	}
	sensString[p] = '\0';

	sens = atof(sensString);
	free(sensString);

	//advance a line
	while( (curChar = getc(fp2) ) != '\n') {}
	autosens = getc(fp2) - 48;

	//advance a line
	while( (curChar = getc(fp2) ) != '\n') {}
	bw = getc(fp2) - 48;

	//advance a line
	while( (curChar = getc(fp2) ) != '\n') {}
	bs = getc(fp2) - 48;

	//advance a line
	while( (curChar = getc(fp2) ) != '\n') {}

	char* colorString = (char*) malloc(20);
	for(p = 0; ((curChar = getc(fp2)) != '#'); p++){
		colorString[p] = curChar;
	}
	colorString[p] = '\0';
	barcolor = colorString;

	//advance a line
	while( (curChar = getc(fp2) ) != '\n') {}

	char* colorBackString = (char*) malloc(20);
	for(p = 0; ((curChar = getc(fp2)) != '#'); p++){
		colorBackString[p] = curChar;
	}
	colorBackString[p] = '\0';
	bkcolor = colorBackString;

	//advance a line
	while( (curChar = getc(fp2) ) != '\n') {}
	char* gravityString = (char*) malloc(10);
	for(p = 0; ((curChar = getc(fp2)) != '#'); p++){
		gravityString[p] = curChar;
	}
	gravityString[p] = '\0';
	gravity = atof(gravityString);

}

//sanity check for values, so values out of bounds don't cause the program to behave incorrectly
void validate_config()
{
	getPulseDefaultSink((void*)&audio);

	// validate bar width between min (1) and max (200)
	if (bw > 200) {
		bw = 200;
	}
	if (bw < 1) {
		bw = 1;
	}

	//color validation
	if (strcmp(barcolor, "black") == 0) { col = 0; }
	if (strcmp(barcolor, "red") == 0) { col = 1; }
	if (strcmp(barcolor, "green") == 0) { col = 2; }
	if (strcmp(barcolor, "yellow") == 0) { col = 3; }
	if (strcmp(barcolor, "blue") == 0) { col = 4; }
	if (strcmp(barcolor, "magenta") == 0) { col = 5; }
	if (strcmp(barcolor, "cyan") == 0) { col = 6; }
	if (strcmp(barcolor, "white") == 0) { col = 7; }
	// default if invalid

	//background color checking
	if (strcmp(bkcolor, "black") == 0) { bgcol = 0; }
	if (strcmp(bkcolor, "red") == 0) { bgcol = 1; }
	if (strcmp(bkcolor, "green") == 0) { bgcol = 2; }
	if (strcmp(bkcolor, "yellow") == 0) { bgcol = 3; }
	if (strcmp(bkcolor, "blue") == 0) { bgcol = 4; }
	if (strcmp(bkcolor, "magenta") == 0) { bgcol = 5; }
	if (strcmp(bkcolor, "cyan") == 0) { bgcol = 6; }
	if (strcmp(bkcolor, "white") == 0) { bgcol = 7; }
	// default if invalid

	//gravity sanity check
	if (gravity < 0) {
		gravity = 0;
	}
	
	//setting sensitivity
	sens = sens / 100;

}

int * separate_freq_bands(fftw_complex out[M / 2 + 1][2], int bars, int lcf[200], int hcf[200], float k[200], int channel) {
	int x,i;
	float peak[201];
	static int fl[200];
	static int fr[200];
	int y[M / 2 + 1];
	float temp;
		
	//separating frequency bands
	for (x = 0; x < bars; x++) {

		peak[x] = 0;

		//getting peaks
		for (i = lcf[x]; i <= hcf[x]; i++) {

			y[i] =  pow(pow(*out[i][0], 2) + pow(*out[i][1], 2), 0.5); //getting r of complex #s
			peak[x] += y[i]; //summing band
			
		}
		
		peak[x] = peak[x] / (hcf[x]-lcf[x]+1); //getting average
		temp = peak[x] * k[x] * sens; //multiplying with k and adjusting to sens settings
		
		if (channel == 1) {
			fl[x] = temp;
		} else {
			fr[x] = temp;
		}	
	}

	if (channel == 1) {
		return fl;
 	} else {
 		return fr;
 	}	
} 

// general: entry point
int main(int argc, char **argv)
{
	// general: define variables
	pthread_t  p_thread;
	int thr_id GCC_UNUSED;
	float fc[200];
	float fre[200];
	int f[200], lcf[200], hcf[200];
	int *fl;
	int fmem[200];
	int flast[200];
	int flastd[200];
	int sleep = 0;
	int i, n, o, height, h, w, c, rest, inAVirtualConsole, silence;
	float temp;
	double inr[2 * (M / 2 + 1)];
	double inl[2 * (M / 2 + 1)];
	fftw_complex outl[M / 2 + 1][2];
	fftw_plan pl;
	int fall[200];
	float fpeak[200];
	float k[200];
	float g;
	struct timespec req = { .tv_sec = 0, .tv_nsec = 0 };
	char configPath[255];
	char ch;
	
	// general: console title
	printf("%c]0;%s%c", '\033', "CIS DEMO VISUALIZER", '\007');
	
	configPath[0] = '\0';

	setlocale(LC_ALL, "");

	// general: handle Ctrl+C
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = &sig_handler;
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);
	

	if((c = getopt(argc, argv, "p:vh")) != -1){
		printf("Error, %s not a valid argument.\n", argv[1]);
		abort();
	}

	//confirm running in virtual console 
	inAVirtualConsole = 1;
	if (strncmp(ttyname(0), "/dev/tty", 8) == 0 || strcmp(ttyname(0), "/dev/console") == 0) {
		inAVirtualConsole = 0;
	}
	
	if (!inAVirtualConsole) {
		system("setfont cmusvis.psf  >/dev/null 2>&1");
		system("echo yep > /tmp/testing123");
		system("setterm -blank 0");
	}
	
	//the main loop!
	while (1) {

	//load config and perform sanity check
	load_config(configPath);
	validate_config();

	//initial input
	audio.format = -1;
	audio.rate = 0;
	audio.terminate = 0;
	audio.channels = 1;

	for (i = 0; i < M; i++) {
		audio.audio_out_l[i] = 0;
		audio.audio_out_r[i] = 0;
	}
	
	//starting the music listener
	thr_id = pthread_create(&p_thread, NULL, input_pulse, (void*)&audio); 
	audio.rate = 44100;

	//getting ready to jam
	pl =  fftw_plan_dft_r2c_1d(M, inl, *outl, FFTW_MEASURE); 

	bool reloadConf = FALSE;
	
	while  (!reloadConf) {//returning back to this loop means that you resized the screen
		for (i = 0; i < 200; i++) {
			flast[i] = 0;
			flastd[i] = 0;
			fall[i] = 0;
			fpeak[i] = 0;
			fmem[i] = 0;
			f[i] = 0;
		}

		//start ncurses output 
		init_terminal_ncurses(col, bgcol);
		
		//ncurses width and height
		get_terminal_dim_ncurses(&w, &h);

		//setting bar size
		bars = (w + bs) / (bw + bs);

		height = h - 1;

		//calculate gravity for smoothing
		g = gravity * ((float)height / 270) * pow((60 / (float)framerate), 2.5);

		//checks if there is empty room
		//used for center ing
		rest = (w - bars * bw - bars * bs + bs) / 2;
		if (rest < 0) {
			rest = 0;
		}

		#ifdef DEBUG
			printw("height: %d width: %d bars:%d bar width: %d rest: %d\n", h, w, bars, bw, rest);
		#endif

		if ((smcount > 0) && (bars > 0)) {
			smh = (double)(((double)smcount)/((double)bars));
		}

		double freqconst = log10((float)lowcf / 
			(float)highcf) / ((float)1 / ((float)bars + (float)1) - 1);

		// process: calculate cutoff frequencies
		for (n = 0; n < bars + 1; n++) {
			//stopped at 10000 ceiling, no point in going above for non scientific visualizer
			fc[n] = highcf * pow(10, freqconst * (-1) + ((((float)n + 1) / ((float)bars + 1)) * freqconst)); 
			//the following was calculated through attempting to use Nyquist
			//my rough calculations led me to believe that rate/2 and nyquist freq in M/2
			//however, testing shows it is not...the following is the result of experimentation more than 
			//precise calculation
			fre[n] = fc[n] / (audio.rate / 2); 
			//lcf contains the lower cut freq for each bar in the FFT out buffer
			lcf[n] = fre[n] * (M /4); 
			if (n != 0) {
				hcf[n - 1] = lcf[n] - 1;
				if (lcf[n] <= lcf[n - 1]) {
					//pushing the spectrum up if exp function issues exist
					lcf[n] = lcf[n - 1] + 1; 
				}
				hcf[n - 1] = lcf[n] - 1;
			}

			#ifdef DEBUG
			 			if (n != 0) {
							mvprintw(n,0,"%d: %f -> %f (%d -> %d) \n", n, fc[n - 1], fc[n], lcf[n - 1],
					 				 hcf[n - 1]);
						}
			#endif
		}

		// process: weigh signal to frequencies
		for (n = 0; n < bars; n++) {
			k[n] = pow(fc[n],0.85) * ((float)height/(M*4000)) * smooth[(int)floor(((double)n) * smh)];	
		}

	   	bool resizeTerminal = FALSE;

		while  (!resizeTerminal) {
			ch = getch();
			switch (ch) {
				case 65:    //up key
					sens = sens * 1.05;
					break;
				case 66:    //down key
					sens = sens * 0.95;
					break;
				case 68:    //right key
					bw++;
					resizeTerminal = TRUE;
					break;
				case 67:    //left
					if (bw > 1) {
						bw--;
					}	
					resizeTerminal = TRUE;
					break;
				case 'r': //reloading config
					should_reload = 1;
					break;
				case 'c': //changing forground color
					if (col < 7) {
						col++;
					} else {
						col = 0;
					}	
					resizeTerminal = TRUE;
					break;
				case 'b': //changing backround color
					if (bgcol < 7) {
						bgcol++;
					} else {
						bgcol = 0;
					}
					resizeTerminal = TRUE;
					break;

				case 'q': //quit
					cleanup();
					return EXIT_SUCCESS;
			}

			if (should_reload) {
				//telling audio thread to terminate
				audio.terminate = 1;
				pthread_join( p_thread, NULL);
				reloadConf = TRUE;
				resizeTerminal = TRUE;
				should_reload = 0;
			}

			#ifdef DEBUG
				refresh();
			#endif

			//fill input buffer.
			//checking if there is input in buffer			
			silence = 1;
			for (i = 0; i < (2 * (M / 2 + 1)); i++) {
				if (i < M) {
					inl[i] = audio.audio_out_l[i];
					if (inl[i] || inr[i]) {
						silence = 0;
					}	 
				} else {
					inl[i] = 0;
				}
			}

			if (silence == 1) {
				sleep++;
			} else {
				sleep = 0;
			}	

			//if input was present for the last 5 seconds, apply FFT 
			if (sleep < framerate * 5) {
				//execute FFT and sort frequency bands
				fftw_execute(pl);
				fl = separate_freq_bands(outl,bars,lcf,hcf, k, 1);
			} else { //if in sleep mode, wait and continue
				#ifdef DEBUG
					printw("no sound detected for 3 sec, going to sleep mode\n");
				#endif
				//wait a second and then check sound again
				req.tv_sec = 1;
				req.tv_nsec = 0;
				nanosleep (&req, NULL);
				continue;
			}

			//smoothing
			//preparing signal for drawing
			for (o = 0; o < bars; o++) {
				f[o] = fl[o];
			}

			// attempting to make falloff
			if (g > 0) {
				for (o = 0; o < bars; o++) {
					temp = f[o];
					if (temp < flast[o]) {
						f[o] = fpeak[o] - (g * fall[o] * fall[o]);
						fall[o]++;
					} else if (temp >= flast[o]) {
						f[o] = temp;
						fpeak[o] = f[o];
						fall[o] = 0;
					}
					flast[o] = f[o];
				}
			}

			//integral smoothing attempt
			if (integral > 0) {
				for (o = 0; o < bars; o++) {
					fmem[o] = fmem[o] * integral + f[o];
					f[o] = fmem[o];

					#ifdef DEBUG
						mvprintw(o,0,"%d: f:%f->%f (%d->%d)peak:%d \n", o, fc[o], fc[o + 1],
									 lcf[o], hcf[o], f[o]);
					#endif
				}
			}

			//handling div/0 segfault
			for (o = 0; o < bars; o++) {
				if (f[o] < 1) {
					f[o] = 1;
				}	
			}

			//audo sensitivity adjustment
			if (autosens) {
				for (o = 0; o < bars; o++) {
					if (f[o] > height * 8) {
						sens = sens * 0.99;
						break;
					}		
				}
			}

			//its drawing time
			#ifndef DEBUG
				rc = draw_terminal_ncurses(inAVirtualConsole, h, w, bars, bw, bs, rest, f, flastd);
				//edited to control ability to resize terminal
				if (rc == -1) {
					resizeTerminal = TRUE; //terminal has been resized breaking to recalibrating values
				}
				if (framerate <= 1) {
					req.tv_sec = 1  / (float)framerate;
				} else {
					req.tv_sec = 0;
					req.tv_nsec = (1 / (float)framerate) * 1000000000; //sleeping for set up
				}

				nanosleep (&req, NULL);
			#endif
		
			for (o = 0; o < bars; o++) {	
				flastd[o] = f[o];
			} 
		}
	}//reloading config
	req.tv_sec = 1; //waiting a second to free audio streams
	req.tv_nsec = 0;
	nanosleep (&req, NULL);
	}
}
