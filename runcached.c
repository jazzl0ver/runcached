/* 
 * runcached
 * Execute commands while caching their output for subsequent calls. 
 * Command output will be cached for $cacheperiod and replayed for subsequent calls
 *
 * Author Spiros Ioannou sivann <at> inaccess.com
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>


int cacheperiod=27; //seconds
char cachedir[512];
int maxwaitprev=5;  //seconds to wait for previous same command to finish before quiting
int minrand=0;      //random seconds to wait before running cmd
int maxrand=0;
int argskip=1;
char pidfile[128];

char * str2md5str( char[]);
void cleanup(void);
int runit(char **,char *,char *,char *,char *);
int isfile(char *path) ;

int main(int argc, char **argv) {
    int i,j,count;
    char cmd[1024];
    char buf[512];
    char * cmdmd5;
    FILE *fp;
    struct stat st;
    time_t diffsec;

    char cmddatafile[128];
    char cmdexitcode[128];
    char cmdfile[128];

    if (argc<2 || (argc==2 && !strcmp(argv[1],"-c"))) {
        fprintf(stderr,"Usage: %s [-c cacheperiod] <command to execute with args>\n",argv[0]);
        exit(1);
    }

    if (!strcmp(argv[1],"-c")) {
        cacheperiod=atoi(argv[2]);
        argskip=3;
    }

    strcpy(cachedir,"/tmp");

    for (j=0,cmd[0]=0,i=0+argskip;i<argc;i++) {
        j+=strlen(argv[i]);
        if (j+1>sizeof(cachedir)) {
            fprintf(stderr,"argument list too long\n");
            exit(2);
        }
        strcat(cmd,argv[i]);
        strcat(cmd," ");
    }

    cmd[strlen(cmd)-1]=0;

    cmdmd5=str2md5str(cmd);

    if (maxrand-minrand) {
        srand(time(NULL));
        sleep(rand()%(maxrand+1)+minrand);
    }

    snprintf(pidfile,127,"%s/%s-runcached.pid",cachedir,cmdmd5);
    snprintf(cmddatafile,127,"%s/%s.data",cachedir,cmdmd5);
    snprintf(cmdexitcode,127,"%s/%s.exitcode",cachedir,cmdmd5);
    snprintf(cmdfile,127,"%s/%s.cmd",cachedir,cmdmd5);

    atexit(cleanup);

    // don't run the same command in parallel, wait the previous one to finish
    count=maxwaitprev;
    while (isfile(pidfile)) {
        sleep(1);
        count-=1;
        if (count == 0) {
            fprintf(stderr,"timeout waiting for previous %s to finish\n" , cmd);
            exit (1);
        }
    }

    //write pid file
    fp = fopen(pidfile,"w");
	if (!fp) {
		perror(pidfile);
		exit(2);
	}
    fprintf(fp,"%ld",(long)getpid());
    fclose(fp);

    //if not cached before, run it
    if (!isfile(cmddatafile)) {
        runit(argv,cmd,cmddatafile,cmdexitcode,cmdfile) ;
    }

    //if too old, re-run it
    if (stat(cmddatafile, &st) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    diffsec=time(0)-(time_t)st.st_mtim.tv_sec;

    if (diffsec > cacheperiod) {
        runit(argv,cmd,cmddatafile,cmdexitcode,cmdfile) ;
    }

    fp = fopen(cmddatafile,"r");
	if (!fp) {
		perror(cmddatafile);
		exit(1);
	}
    while (fgets(buf, sizeof(buf), fp) != NULL)
        printf("%s", buf);
    fclose(fp);

    exit(0);

} //main

int isfile(char *path) {
    return (access( path, F_OK ) != -1 ) ;
}

void cleanup(void) {
    //if ( access( pidfile, F_OK ) != -1 ) {
	printf("Cleanup\n");
    if (isfile(pidfile)) {
        unlink(pidfile);
    }
}

int runit(char **argv,char * cmd,char * cmddatafile,char * cmdexitcode,char * cmdfile) {
    int out_old, out_new;
    int err_old, err_new;
    int exitcode;
    char buf[1024];

    FILE *pfp, *fp;


    fflush(stdout);
    fflush(stderr);

    //redirect stdout and stderr to file
    out_old = dup(1);
    out_new = open(cmddatafile,O_WRONLY|O_CREAT|O_TRUNC,S_IROTH|S_IRUSR|S_IRGRP|S_IWUSR);
    dup2(out_new, 1);
    close(out_new);

    err_old = dup(2);
    err_new = open(cmddatafile,O_WRONLY|O_CREAT|O_APPEND,S_IROTH|S_IRUSR|S_IRGRP|S_IWUSR);
    dup2(err_new, 2);
    close(err_new);

    /* run command with arguments: */
    /*
    argv+=argskip;
    if (execvp(*argv, argv)<0) {
        perror("execvp");
        exit (errno);
    }
    */

    pfp = popen(cmd, "r");
    if (pfp == NULL) {
        perror("popen");
        exit(errno);
    }

    while (fgets(buf, sizeof(buf), pfp) != NULL)
        printf("%s", buf);

    exitcode=pclose(pfp);

    fp = fopen(cmdexitcode,"w");
	if (!fp) {
		perror(cmdexitcode);
	}
	else {
		fprintf(fp,"%d",exitcode);
		fclose(fp);
	}

    fp = fopen(cmdfile,"w");
	if (!fp) {
		perror(cmdfile);
	}
	else {
		fprintf(fp,"%s",cmd);
		fclose(fp);
	}

    //redirect stdout and stderr back to where it was
    fflush(stdout);
    dup2(out_old, 1);
    close(out_old);

    fflush(stderr);
    dup2(err_old, 2);
    close(err_old);
}


 
typedef union uwb {
    unsigned w;
    unsigned char b[4];
} WBunion;
 
typedef unsigned Digest[4];
 
unsigned f0( unsigned abcd[] ){
    return ( abcd[1] & abcd[2]) | (~abcd[1] & abcd[3]);}
 
unsigned f1( unsigned abcd[] ){
    return ( abcd[3] & abcd[1]) | (~abcd[3] & abcd[2]);}
 
unsigned f2( unsigned abcd[] ){
    return  abcd[1] ^ abcd[2] ^ abcd[3];}
 
unsigned f3( unsigned abcd[] ){
    return abcd[2] ^ (abcd[1] |~ abcd[3]);}
 
typedef unsigned (*DgstFctn)(unsigned a[]);
 
unsigned *calcKs( unsigned *k)
{
    double s, pwr;
    int i;
 
    pwr = pow( 2, 32);
    for (i=0; i<64; i++) {
        s = fabs(sin(1+i));
        k[i] = (unsigned)( s * pwr );
    }
    return k;
}
 
// ROtate v Left by amt bits
unsigned rol( unsigned v, short amt )
{
    unsigned  msk1 = (1<<amt) -1;
    return ((v>>(32-amt)) & msk1) | ((v<<amt) & ~msk1);
}
 
unsigned *md5( const char *msg, int mlen) 
{
    static Digest h0 = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };
//    static Digest h0 = { 0x01234567, 0x89ABCDEF, 0xFEDCBA98, 0x76543210 };
    static DgstFctn ff[] = { &f0, &f1, &f2, &f3 };
    static short M[] = { 1, 5, 3, 7 };
    static short O[] = { 0, 1, 5, 0 };
    static short rot0[] = { 7,12,17,22};
    static short rot1[] = { 5, 9,14,20};
    static short rot2[] = { 4,11,16,23};
    static short rot3[] = { 6,10,15,21};
    static short *rots[] = {rot0, rot1, rot2, rot3 };
    static unsigned kspace[64];
    static unsigned *k;
 
    static Digest h;
    Digest abcd;
    DgstFctn fctn;
    short m, o, g;
    unsigned f;
    short *rotn;
    union {
        unsigned w[16];
        char     b[64];
    }mm;
    int os = 0;
    int grp, grps, q, p;
    unsigned char *msg2;
 
    if (k==NULL) k= calcKs(kspace);
 
    for (q=0; q<4; q++) h[q] = h0[q];   // initialize
 
    {
        grps  = 1 + (mlen+8)/64;
        msg2 = malloc( 64*grps);
        memcpy( msg2, msg, mlen);
        msg2[mlen] = (unsigned char)0x80;  
        q = mlen + 1;
        while (q < 64*grps){ msg2[q] = 0; q++ ; }
        {
//            unsigned char t;
            WBunion u;
            u.w = 8*mlen;
//            t = u.b[0]; u.b[0] = u.b[3]; u.b[3] = t;
//            t = u.b[1]; u.b[1] = u.b[2]; u.b[2] = t;
            q -= 8;
            memcpy(msg2+q, &u.w, 4 );
        }
    }
 
    for (grp=0; grp<grps; grp++)
    {
        memcpy( mm.b, msg2+os, 64);
        for(q=0;q<4;q++) abcd[q] = h[q];
        for (p = 0; p<4; p++) {
            fctn = ff[p];
            rotn = rots[p];
            m = M[p]; o= O[p];
            for (q=0; q<16; q++) {
                g = (m*q + o) % 16;
                f = abcd[1] + rol( abcd[0]+ fctn(abcd) + k[q+16*p] + mm.w[g], rotn[q%4]);
 
                abcd[0] = abcd[3];
                abcd[3] = abcd[2];
                abcd[2] = abcd[1];
                abcd[1] = f;
            }
        }
        for (p=0; p<4; p++)
            h[p] += abcd[p];
        os += 64;
    }
    return h;
}    
 
char * str2md5str( char msg[] )
{
    int j,k;
    unsigned *d = md5(msg, strlen(msg));
    WBunion u;
    char s[16];

    char * md5str;
    md5str=malloc(34);
 
    for (j=0;j<4; j++){
        u.w = d[j];
        for (k=0;k<4;k++)  {
            sprintf(s,"%02x",u.b[k]);
            strcat(md5str,s);
        }
    }
 
    return md5str;
}
