/*
    SHM.C:  Shape manager for

    CGA/EGA/VGA/CGA GRAY/EGA GRAY

    Handles the following functions:

    shm_init :Init shape manager
    shm_do   :Prepare shape tables (load/trash) using shm_want[]
    shm_exit :Terminate shape manager

*/

#include <stdlib.h>;
#include <io.h>;
#include <alloc.h>;
#include <fcntl.h>;
#include <string.h>;
#include <math.h>;
#include <mem.h>;
#include "gr.h";

extern void rexit (int num);

int   shm_want    [shm_maxtbls];
char  *shm_tbladdr [shm_maxtbls];
int   shm_tbllen   [shm_maxtbls];
int   shm_flags    [shm_maxtbls];

char  shm_fname    [80];
char  colortab[256];

const char egatab[256]={
   /*0*/ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
   /*1*/ 0,8,8,7,7,7,15,15,0,4,12,12,8,8,2,6,
   /*2*/ 6,12,2,2,2,6,6,14,2,2,2,2,6,14,10,10,
   /*3*/ 10,14,14,10,10,10,14,14,0,0,0,4,4,5,0,0,
   /*4*/ 0,4,12,12,8,8,7,6,4,12,2,2,2,6,6,6,
   /*5*/ 2,2,2,2,6,6,10,10,10,14,14,10,10,10,14,14,
   /*6*/ 0,0,4,4,12,12,0,0,4,4,12,12,8,8,8,6,
   /*7*/ 6,12,2,2,8,8,6,6,2,2,2,2,6,6,10,10,
   /*8*/ 10,14,14,9,9,9,14,14,1,1,5,5,4,2,1,1,
   /*9*/ 5,5,13,13,1,1,8,5,5,12,3,3,7,7,7,12,
   /*a*/ 3,3,3,7,7,14,2,2,3,7,7,10,10,10,14,14,
   /*b*/ 1,1,1,5,13,13,1,1,1,5,13,13,1,1,5,5,
   /*c*/ 13,13,9,9,9,5,5,14,3,3,3,7,7,15,4,4,
   /*d*/ 11,7,7,11,11,11,15,15,9,9,9,5,13,13,9,9,
   /*e*/ 9,5,13,13,9,9,9,5,5,13,9,9,9,9,5,13,
   /*f*/ 11,11,3,3,13,13,11,11,13,15,13,13,13,15,15,0};

void shm_init (char *fname) {
   int c;
   strcpy (shm_fname, fname);
   for (c=0; c<shm_maxtbls; c++) {
      shm_want   [c] = 0;
      shm_tbladdr [c] = NULL;
      }
   }

void init8bit (void) { /* Init pallette for numcolorbits=8 */
   int x;
   /**/
   switch (x_modetype) {
      case x_cga:
	 for (x=0; x<256; x++) {colortab[x]=x&3;};
	 break;
      case x_ega:
	 memcpy (&colortab,&egatab,256);
	 break;
      case x_vga:
	 for (x=0; x<256; x++) {colortab[x]=x;};
	 break;
      };
   for (x=0; x<256; x++) {
      colortab[x]=x;
      };
   };

#define readin(dest,addr,len) memcpy (&dest,addr,len); addr+=len
#define wr(addr,ofs,src,len) memcpy (addr+ofs,&src,len); ofs+=len
#define wr1(addr,ofs,byt) *(addr+(ofs++))=byt

void xlate_table (int n, char *addr, char *bucket1) {
   unsigned char  numshapes=0;
   char  numcolorbits=1;
   int   numrots;
   long  longcolor;
   int   len_cga, len_ega, len_vga, len;
   byte  xs, xsb, ys;
   byte  storetype;
   byte  c;
   int   x,y,b,flags;
   char  *bucket,*tempbucket;
   unsigned char  databyte,shapebyte;
   unsigned int   colorand, colorshift;
   char  *dest;
   int   tblofs, dataofs;
   /**/
   readin (numshapes, addr, 1);
   readin (numrots, addr, 2);
   readin (len_cga, addr, 2);
   readin (len_ega, addr, 2);
   readin (len_vga, addr, 2);
   readin (numcolorbits, addr, 1);
   readin (flags, addr, 2);
   if (x_ourmode==x_cga) {
      len        = len_cga+16;
      colorand   = 3;
      colorshift = 0;
      }
   else if (x_ourmode==x_cgagrey) {
      len        = len_cga+16;
      colorand   = 3;
      colorshift = 4;
      }
   else if (x_ourmode==x_ega) {
      len        = len_ega+16;
      colorand   = 15;
      colorshift = 8;
      }
   else {
      len        = len_vga+16;
      colorand   = 255;
      colorshift = 16;
      }
   /**/
   if (flags&shm_fontf) {
      for (c=0; c<(numcolors); c++) colortab[c] = c;
      }
   else if (numcolorbits==8) init8bit();
   else {
      for (c=0; c<numcolors; c++) {  /* Numcolors is a macro */
	 readin (longcolor, addr, 4);
	 colortab [c] = (longcolor >> colorshift) & colorand;
	 };
      };
   /**/
   dest = malloc (len);
   if (dest==NULL) {
      rexit (9);}
   shm_tbllen  [n] = len;
   shm_tbladdr [n] = dest;
   shm_flags   [n] = flags;
   tblofs          = 0;
   dataofs	   = numshapes*4;
   /**/
   for (c=0; c<numshapes; c++) {
      readin (xs, addr, 1);
      readin (ys, addr, 1);
      readin (storetype, addr, 1);
      bucket=bucket1;
      switch (storetype) {
	 case st_byte:
	    memcpy (bucket, addr, xs*ys);
	    (char*) addr += (xs*ys);
	    break;
	 case st_plain:
	    break;
	 case st_rle:
	    break;
	 }
      /*
	 Now the shape definition is in memory at BUCKET
	 Translate to our mode:
      */
      if ((numcolorbits==8)&&(xs==64)&&(ys==12)) {
	 if (x_ourmode==x_vga) {
	    memmove (&vgapal,bucket,sizeof (vgapal));
	    vga_setpal();
	    }
	 else {
	    };
	 };
      if ((x_ourmode==x_cga)||(x_ourmode==x_cgagrey)) {
	 xsb = (xs+3)/4;
	 wr (dest, tblofs, dataofs, 2);
	 wr1 (dest, tblofs,  xsb);
	 wr1 (dest, tblofs,  ys);
	 /**/
	 shapebyte = 0;
	 for (y=0; y<ys; y++) {
	    for (x=0; x<xs; x++) {
	       databyte = colortab [(*(byte*)(bucket + x + y*xs))];
	       shapebyte |= (databyte << (6-2*(x&3)));
	       if (((x&3)==3) || (x==(xs-1))) {
		  wr1 (dest, dataofs, shapebyte);
		  shapebyte = 0;
		  }
	       }
	    }
	 }
      else if ((x_ourmode==x_ega)||(x_ourmode==x_egagrey)) {
	 xsb = xs;
	 wr (dest, tblofs,  dataofs, 2);
	 wr1 (dest, tblofs,  xsb);
	 wr1 (dest, tblofs,  ys);
	 /**/
	 if (!(flags&shm_blflag)) {
	    for (y=0; y<ys; y++) {
	       for (x=0; x<xs; x+=2) {
		  shapebyte =
		     (colortab [(*(byte*)(bucket+x+  y*xs))]) |
		     (colortab [(*(byte*)(bucket+x+1+y*xs))]<<4);
		  wr1 (dest,dataofs, shapebyte);
		  };
	       };
	    }
	 else {
	    for (b=8; b>0; b>>=1) { /* plane */
	       for (y=0; y<ys; y++) {
		  shapebyte=0;
		  for (x=0; x<xs; x++) {
		     databyte=colortab[(*(byte*)(bucket+x+y*xs))];
		     shapebyte|=
		       ((databyte&b)!=0)<<(7-(x&7));
		     if (((x&7)==7)||(x==(xs-1))) {
			wr1 (dest,dataofs, shapebyte);
			shapebyte=0;
			};
		     };
		  };
	       };
	    };
	 }
      else {   /* vga */
	 xsb = xs;
	 wr (dest, tblofs, dataofs, 2);
	 wr1 (dest, tblofs, xsb);
	 wr1 (dest, tblofs, ys);
	 /**/
	 if (!(flags&shm_blflag)) {
	    for (y=0; y<ys; y++) {
	       for (x=0; x<xs; x++) {
		  shapebyte = colortab [(*(byte*)(bucket + x + y*xs))];
		  wr1 (dest, dataofs, shapebyte);
		  };
	       };
	    }
	 else {
	    for (b=3; b>=0; b--) {
	       for (y=0; y<ys; y++) {
		  for (x=0; x<xs; x+=4) {
		     shapebyte=colortab[*(byte*)(bucket+(x+b)+y*xs)];
		     wr1 (dest,dataofs,shapebyte);
		     };
		  };
	       };
	    };
	 };
      /*if (dataofs>=len) exit(99); */
      };
   };

void shm_do (void) {
   long shoffset [128];
   char *shaddr  [128];
   int  shlen    [128]; /* Dual purpose */
   int  shafile;
   int  c;
   char *bucket1;
   /**/
   bucket1 = malloc (4096);
   if (bucket1==NULL) {
      rexit (9);}
   /**/
   for (c=0; c<shm_maxtbls; c++) {
      shaddr [c] = NULL;
      };
   /**/
   shafile = _open (shm_fname,O_BINARY);
   if (!read (shafile,shoffset,128*4)) {
      rexit(9);
      }
   read (shafile,shlen,128*2);
   /**/
   for (c=0; c<shm_maxtbls; c++) {
      if (!shm_want[c]) {
	 if (shm_tbladdr [c] != NULL) {
	    free (shm_tbladdr [c]);
	    shm_tbladdr [c] = NULL;
	    }
	 }
      else if ((shm_tbladdr[c] == NULL)&&(shlen[c]!=0)) {
	 lseek (shafile, shoffset [c], SEEK_SET);
	 shaddr [c] = malloc (shlen [c]);
	 if (shaddr [c] == NULL) {
	    rexit (9);}
	 read (shafile, shaddr [c], shlen[c]);
	 }
      }
   close (shafile);
   /**/
   for (c=0; c<shm_maxtbls; c++) {
      if (shaddr [c] != NULL) {
	 xlate_table (c,shaddr [c], bucket1);
	 free (shaddr [c]);
	 }
      }
   free (bucket1);
   }

void shm_exit (void) {
   int c;
   for (c=0; c<shm_maxtbls; c++) {
      if (shm_tbladdr [c] != NULL) {
		 free (shm_tbladdr [c]);
		 shm_tbladdr [c] = NULL;
		}
	}
	}
