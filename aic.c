#include <stdio.h>

/* 文字列sから文字cが出現するまでを文字列dとして返す */
/* タブ（0x09）やスペース（0x20）は無視する          */
/* 戻り値として文字cの出現位置を返す                 */
/* 文字cが見つからない場合は、それまでに出現した文字 */
/* 列dを返し、戻り値として0を返す                    */
/* シフトJIS（第1バイト 81～9F,E0～EF、第2バイト 40  */
/* ～7E,80～FC）は意識しないので、文字列sにシフトJIS */
/* コードが含まれる場合は、文字cはシフトJISの第2バイ */
/* トに該当しないのが望ましい                        */
int strdiv(char *d,char *s,char c) {
	int i,j;

	i=0;
	j=0;
	while(s[i]!=c) {
		if(s[i]=='\0') {
			i=0;
			break;
		}
		if(s[i]!=0x09 && s[i]!=0x20) {
			d[j]=s[i];
			j++;
		}
		i++;
	}
	d[j]='\0';
	return(i);
}

int tovalue(char c) {
	int i;
	char s[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	i=0;
	while(s[i]!=c) {
		if(s[i]=='\0') {
			i=0;
			break;
		}
		i++;
	}
	return(i);
}

char *fgetms(char *s,int n,FILE *fp) {
	int i;
	char buf[1],c;

	c=0;
	i=0;
	while(i<n) {
		if(fread(buf,1,1,fp)<1) {
			s[i]='\0';
			s=NULL;
			break;
		}
		if(c==0x0d && buf[0]==0x0a) {
			s[i-1]='\0';
			break;
		}
		c=buf[0];
		s[i]=buf[0];
		i++;
	}
	return(s);
}

main() {
	int ss;
	int ax,ay,bx,by,sx,sy,tx,ty,fx,fy,cx,cy,sv,fv,kv,rv,gv,bv;
	int i,j;
	char s[256],d[256],fn[256];
	FILE *fp;

	tx=26;
	ty=20;
	fx=fy=24;
	ax=ay=bx=by=sx=sy=cx=cy=sv=fv=kv=rv=gv=bv=0;
	printf("*define\n");
	printf("game\n");
	printf("*start\n");
	fp=fopen("GAMEEXE.INI","rb");
	while(fgetms(s,256,fp)!=NULL) {
		i=strdiv(d,s,'=');
/*		printf("(%s),(%d)\n",d,i);	*/
		if(strcmp(d,"#SEEN_START")==0) {
			sscanf(s+i+1,"%03d",&ss);
		}
		if(strcmp(d,"#WINDOW_ATTR_AREA")==0) {
			sscanf(s+i+1,"%03d,%03d,%03d,%03d",&ax,&ay,&bx,&by);
		}
		if(strcmp(d,"#WINDOW_MSG_POS")==0) {
			sscanf(s+i+1,"%03d,%03d",&sx,&sy);
		}
		if(strcmp(d,"#MESSAGE_SIZE")==0) {
			sscanf(s+i+1,"%03d,%03d",&tx,&ty);
		}
		if(strcmp(d,"#FONT_SIZE")==0) {
			sscanf(s+i+1,"%03d",&fx);
			fy=fx;
		}
		if(strcmp(d,"#MSG_MOJI_SIZE")==0) {
			sscanf(s+i+1,"%03d,%03d",&cx,&cy);
			cx=cx-(fx/2);
			cy=cy-fy;
		}
		if(strcmp(d,"#MSG_SPEED")==0) {
			sscanf(s+i+1,"%03d",&sv);
		}
		if(strcmp(d,"#MOJI_KAGE")==0) {
			sscanf(s+i+1,"%03d",&kv);
			if(kv==0) {
				kv=1;
			}
			else {
				kv=0;
			}
		}
		if(strcmp(d,"#WINDOW_ATTR")==0) {
			sscanf(s+i+1,"%03d,%03d,%03d",&rv,&gv,&bv);
		}
		if(strcmp(d,"#CAPTION")==0) {
			printf("caption %s\n",s+i+1);
		}
		if(strcmp(d,"#USEFONT")==0) {
/*			printf("defaultfont %s\n",s+i+1);	*/	/* 定義ブロックのみ */
		}
		if(strncmp(d,"#NAME.",6)==0) {
			printf("mov $%d,%s\n",27+tovalue(d[6])*26+tovalue(d[7]),s+i+1);
		}
	}
	printf("setwindow %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,#%02X%02X%02X,%d,%d,%d,%d\n",sx+ax+18,sy+ay+6,tx,ty,fx,fy,cx,cy,sv,fv,kv,rv,gv,bv,sx+ax,sy+ay,sx+ax+((fx+cx)*tx)+18+22,sy+ay+((fy+cy)*ty)+6+2);
	printf("goto *seen%03d\n",ss);
}
