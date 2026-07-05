#include <stdio.h>
#include <string.h>

#define BUF_SIZE 16384
#define STR_SIZE 512
#define DIM_SIZE 32
#define TBL_SIZE 8192

struct data {
	int an;				/* コマンドのアドレス幅 */
	unsigned long am;		/* シナリオサイズ（最終アドレス＋１） */
	unsigned long ad;		/* コマンドの開始アドレス */

	int rn;				/* 読み込みデータの読み取り位置 */
	int rm;				/* 読み込みデータの大きさ */
	unsigned char rd[BUF_SIZE];	/* 読み込みデータ */

	int pn;				/* パラメータの読み取り位置 */
	char pf[STR_SIZE];		/* パラメータ処理中のフラグ */
	unsigned long pm;		/* パラメータの繰り返し回数 */
	int pc;				/* パラメータの繰り返し位置 */
	int px;				/* パラメータ末尾確認フラグ */

	int sn;				/* 文字列のカウンタ */
	char sd[DIM_SIZE][STR_SIZE];	/* 文字列データ */
	int sc;				/* 文字列配列の使用サイズ */

	int vn;				/* 可変長数値のカウンタ */
	unsigned char vm;		/* 可変長数値の長さ */
	unsigned long vd[DIM_SIZE];	/* 可変長数値データ */
	unsigned char vf[DIM_SIZE];	/* 可変長数値フラグ */
	int vc;				/* 可変長数値配列の使用サイズ */

	int en;				/* 式のカウンタ */
	unsigned char ed[DIM_SIZE];	/* 式データ */
	int ec;				/* 式データの使用サイズ */

	unsigned long jd[TBL_SIZE];	/* ジャンプ先データ */
	int jc;				/* ジャンプ先データの使用サイズ */

	int lf;				/* 改行フラグ */
	int lc;				/* 1つ前の改行フラグ */
};

/* 改行フラグをセットする */
char *lfs(struct data *dp,char *s) {
	dp->lf=1;
	return(s);
}

/* 改行フラグをリセットする */
char *lfr(struct data *dp,char *s) {
	dp->lf=0;
	return(s);
}

/* 数値配列の示された範囲をセパレータを挟んで10進数文字列化する */
char *vtos(struct data *dp,char *s,int n,int m,char *p) {
	int i;
	char t[STR_SIZE];

	for(i=n;i<m;i++) {
		if(i==n) sprintf(s,"%ld",dp->vd[i]);
		else {
			sprintf(t,"%s%ld",p,dp->vd[i]);
			strcat(s,t);
		}
	}
	return(s);
}

/* 文字列配列の示された範囲をセパレータを挟んで結合する */
char *stos(struct data *dp,char *s,int n,int m,char *p) {
	int i;
	char t[STR_SIZE];

	for(i=n;i<m;i++) {
		if(i==n) sprintf(s,"%s",dp->sd[i]);
		else {
			sprintf(t,"%s%s",p,dp->sd[i]);
			strcat(s,t);
		}
	}
	return(s);
}

/* リトルエンディアンな数値を読み込む			*/
/* 入力値 n…動作（0…通常、1…アドレス、2…カウンタ）	*/
/*        m…長さ（0なら可変長）			*/
/* 戻り値 0…正常終了、1…読み取り中、-1…エラー	*/
int readval(struct data *dp,char n,char m) {
	int i;

	if(dp->vn==0) {					/* 1バイト目の処理 */
		if(m<1) {					/* 可変長 */
			dp->vm=(dp->rd[dp->rn]&0x70)>>4;		/* バイト長を求める */
			if(dp->vm==0 || dp->vm>5) return(-1);		/* バイト長が0か5以上ならエラー */
			if(dp->vc>=DIM_SIZE) {				/* 配列容量を超えたらエラーで強制終了 */
				printf("*** VALUE TABLE OVER FLOW ***\n");
				exit(1);
			}
			dp->vf[dp->vc]=(dp->rd[dp->rn]&0x80)>>7;	/* フラグを求める */
			dp->vd[dp->vc]=0L|dp->rd[dp->rn]&0x0f;		/* 1バイト目の数値を求める */
			dp->vn=4;					/* 1バイト目の数値サイズは4ビット */
		}
		else {						/* 固定長 */
			dp->vm=m;					/* バイト長 */
			if(dp->vc>=DIM_SIZE) {				/* 配列容量を超えたらエラーで強制終了 */
				printf("*** VALUE TABLE OVER FLOW ***\n");
				exit(1);
			}
			dp->vd[dp->vc]=0L|dp->rd[dp->rn];		/* 1バイト目の数値を求める */
			dp->vn=8;					/* 1バイト目の数値サイズは8ビット */
		}
	}
	else {						/* 2バイト目以降の処理 */
		if(dp->vc>=DIM_SIZE) {				/* 配列容量を超えたらエラーで強制終了 */
			printf("*** VALUE TABLE OVER FLOW ***\n");
			exit(1);
		}
		dp->vd[dp->vc]=dp->vd[dp->vc]|dp->rd[dp->rn]<<dp->vn;	/* 前回読み込んだサイズ分シフトして追加 */
		dp->vn=dp->vn+8;					/* 2バイト目以降の数値サイズは8ビット */
	}
	dp->vm--;					/* バイト長を1つ減らす */
	if(dp->vm!=0) return(1);			/* バイト長が0でなければ次のデータを読み取る為に戻る */
	if(n==2) {					/* カウンタなら保存する */
		if(dp->vd[dp->vc]==0) return(-1);	/* ただし、カウンタが 0 ならエラー */
		dp->pm=dp->vd[dp->vc];
	}
	if(n==1) {					/* アドレスならジャンプテーブルに保存する */
		if(dp->vd[dp->vc]>=dp->am) return(-1);	/* 最終アドレスを超えていたらエラー */
		for(i=0;i<dp->jc;i++) if(dp->jd[i]==dp->vd[dp->vc]) break;
		if(i>=dp->jc) {
			if(dp->jc>=TBL_SIZE) {			/* 配列容量を超えたらエラーで強制終了 */
				printf("*** JUMP TABLE OVER FLOW ***\n");
				exit(1);
			}
			dp->jd[dp->jc++]=dp->vd[dp->vc];
		}
	}
	dp->vn=0;					/* 次のために数値サイズの初期値を0に戻す */
	dp->vc++;					/* 可変長数値の配列番号を1つ進める */
	return(0);
}

/* 文字列を読み込む（末尾は0x00）    */
/* 戻り値 0…正常終了、1…読み取り中 */
int readstr(struct data *dp) {
	if(dp->sc>=DIM_SIZE) {		/* 配列容量を超えたらエラーで強制終了 */
		printf("*** STRING TABLE OVER FLOW ***\n");
		exit(1);
	}
	if(dp->sn>=STR_SIZE) {		/* 配列容量を超えたらエラーで強制終了 */
		printf("*** STRING OVER FLOW ***\n");
		exit(1);
	}

	if(dp->rd[dp->rn]==0x00) {
		dp->sd[dp->sc][dp->sn]='\0';	/* 0x00なら'\0'を書き込んで文字列終了 */
		dp->sn=0;			/* 次に備えて文字列のカウンタを初期化する */
		dp->sc++;			/* 文字列配列のカウンタを進める */
		return(0);
	}
	else {
		dp->sd[dp->sc][dp->sn]=dp->rd[dp->rn];
		dp->sn++;			/* 文字列のカウンタを進める */
		return(1);
	}
}

/* 式を読み取る */
int readexp(struct data *dp) {
/*			else if(cmdcmp(&dp,"15 ee aa")==0) sprintf(s,"if expr goto %ld",dp.vd[0]);	*/
/* ee は式を表す											*/
/* (  は演算子として dp.ed[dp.ec] に順次格納される							*/
/* )  は演算子として dp.ed[dp.ec] に順次格納される							*/
/* &  は演算子として dp.ed[dp.ec] に順次格納される							*/
/* その他の演算子も dp.ed[dp.ec] に順次格納される							*/
/* その他の演算子で必要となる値は dp.vd[dp.vc] に順次格納される						*/
/* ジャンプ先は数値配列の末尾である dp.vd[dp.vc-1] に格納されることになる				*/
/*													*/
/* NScripterの式に変換する際は・・・									*/
/* 順番としては先にカッコつぶしをしておく								*/
/* ((a=b&c=d)&(a=f)) → a=b&c=d を評価して x にする → (x&(a=f)) → a=f を評価して y にする → (x&y)	*/
/* → x&y を評価して z にする → 完了（その他の例は以下）						*/
/* (a=b&c=d) → a=b&c=d → x → 完了									*/
/* (a=b)     → a=b     → x → 完了									*/
/* (a&(b=c)) → b=c     → (a&x) → a&x → y → 完了							*/
	int rc;
	switch(dp->rd[dp->rn]) {
	case 0x37:
	case 0x3B:
	case 0x45:
	case 0x46:
	case 0x47:
	case 0x48:
		switch(dp->en) {
		case 0:
			dp->ed[dp->ec++]=dp->rd[dp->rn++];
			dp->en++;
			break;
		case 1:
			rc=readval(dp,0,0);
			if(rc==0) dp->en++;
			break;
		case 2:
			rc=readval(dp,0,0);
			if(rc==0) dp->en=0;
		}
	case '(':
	case ')':
	case '&':
		dp->ed[dp->ec++]=dp->rd[dp->rn++];
		break;
	default:
		return(-1);
	}

/* (  は演算子として dp.ed[dp.ec] に順次格納される							*/
/* )  は演算子として dp.ed[dp.ec] に順次格納される							*/
/* &  は演算子として dp.ed[dp.ec] に順次格納される							*/
/* その他の演算子も dp.ed[dp.ec] に順次格納される							*/
/* その他の演算子で必要となる値は dp.vd[dp.vc] に順次格納される						*/
/* ジャンプ先は数値配列の末尾である dp.vd[dp.vc-1] に格納されることになる				*/
/* IF [37] xx=yy & [38] xx=yy goto aaaa */
/* (、)、&以外の演算式が1つも現れなかったらエラー */
/* )の手前には(が必要 */
/* &の手前には演算式が1つ以上必要 */
/* NScripter で if %0=0 && %1=0 && %2=0 puttext "エラー"	*/
/* 同様に       if %0=0 if %1=0 if %2=0 puttext "エラー"	*/
/* ただし、&&（AND）はあるが、OR はない。			*/
	return(0);
}

/* 16進数2桁の文字列とデータが一致するか確認する */
/* 戻り値 0…正常終了、-1…エラー                */
int readhex(struct data *dp,char *s) {
	int i;	/* 適当なintでの定義が無いとreturn時に何故か強制終了してしまう */
	unsigned char c;

	sscanf(s,"%02X",&c);			/* 文字列から16進数2桁を読み込む */
	if(dp->rd[dp->rn]!=c) return(-1);	/* 比較して違ったらエラー終了 */
	return(0);
}

/* コマンド解析パラメータとデータが一致するか確認する         */
/* 入力値 ps…パラメータ文字列（2文字ずつ、スペースで区切る） */
/* 戻り値 0…正常終了、1…読み取り中、-1…エラー              */
int cmdcmp(struct data *dp,char *ps,int po) {
	int i;
	int rc;
	char s[256];

	if(strcmp(dp->pf,"")==0) {	/* 処理中でなければ処理内容を入れて変数初期化 */
		strcpy(dp->pf,ps);	/* 処理中フラグを設定 */
		dp->pn=0;		/* パラメータ読み出し位置を初期化 */
		dp->sn=0;		/* 文字列のカウンタ初期化 */
		dp->sc=0;		/* 文字列配列の使用サイズ初期化 */
		dp->vn=0;		/* 可変長数値のカウンタ初期化 */
		dp->vc=0;		/* 文字列配列の使用サイズ初期化 */
		dp->pm=0;		/* パラメータの繰り返し回数初期化 */
		dp->pc=0;		/* パラメータ繰り返し位置初期化 */
		dp->px=0;		/* パラメータ末尾確認フラグ初期化 */
		dp->en=0;
		dp->ec=0;		/* 式データの使用サイズ初期化 */
	}
	if(strcmp(dp->pf,ps)!=0) return(-1);	/* 他の処理を行っていたら何もせずに戻る */

	if(ps[dp->pn]=='(' && ps[dp->pn+1]=='(') {	/* カッコ始まりなら… */
		dp->pn=dp->pn+2;			/* パラメータを2つ進めて… */
		if(ps[dp->pn]==' ') dp->pn++;		/* スペースなら1つ進めて… */
		dp->pc=dp->pn;				/* 繰り返し位置を保存する */
	}
	sscanf(ps+dp->pn,"%2s",s);		/* パラメータから2文字取り出す */

	if(strcmp(s,"ss")==0) rc=readstr(dp);		/* 文字列 */
	else if(strcmp(s,"aa")==0) rc=readval(dp,1,4);	/* 4バイト固定長アドレス値 */
	else if(strcmp(s,"dd")==0) rc=readval(dp,0,4);	/* 4バイト固定長数値 */
	else if(strcmp(s,"ww")==0) rc=readval(dp,0,2);	/* 2バイト固定長数値 */
	else if(strcmp(s,"bb")==0) rc=readval(dp,0,1);	/* 1バイト固定長数値 */
	else if(strcmp(s,"nn")==0) rc=readval(dp,2,1);	/* 1バイト固定長繰り返し値 */
	else if(strcmp(s,"vv")==0) rc=readval(dp,0,0);	/* 可変長数値 */
	else if(strcmp(s,"ee")==0) rc=readexp(dp);	/* 式 */
	else rc=readhex(dp,s);				/* 16進数2桁 */

	if(rc<0 && dp->px>0) {		/* エラーでもパラメータ末尾確認フラグがONなら…	*/
		dp->pn=dp->pc;		/* パラメータ読み取り位置を (( まで戻して、	*/
		dp->rn=dp->px;		/* データ読み取り位置を元に戻して、		*/
		dp->px=0;		/* パラメータ末尾確認フラグをOFFにして、	*/
		rc=0;			/* 正常終了したことにする			*/
	}
	else {
		if(rc<0) {				/* パラメータ処理でエラーが発生したらエラー */
			dp->rn=0;			/* 読み込みデータ読み取り位置を初期化 */
			strcpy(dp->pf,"");		/* 処理中フラグを初期化 */
			return(-1);
		}

		dp->rn++;				/* 読み込みデータの読み取り位置を進める */
		if(rc>0) return(1);			/* 読み取り中なら次のデータを読むために戻る */

		dp->pn=dp->pn+2;			/* パラメータ読み出し位置を次へ進める */
	}

	if(dp->pf[dp->pn]==' ') dp->pn++;	/* 次がスペースなら1つ次へ進める */

	if(ps[dp->pn]==')' && ps[dp->pn+1]==')') {	/* カッコ閉じなら… */
		if(dp->pm>0) dp->pm--;			/* 繰り返し回数が有るなら、繰り返し回数を減らす */
		else dp->px=dp->rn;			/* 繰り返し回数が無いなら、末尾確認フラグをONにする */
		if(dp->pm>0) dp->pn=dp->pc;		/* 繰り返し回数が1回以上ならカッコ始まりまで戻す */
		else {					/* 繰り返し回数が終わりなら… */
			dp->pn=dp->pn+2;		/* パラメータを2つ進めて… */
			if(dp->pf[dp->pn]==' ') dp->pn++;	/* 次がスペースなら1つ次へ進める */
		}
	}

	if(ps[dp->pn]!='\0') return(1);		/* パラメータ末尾でなければ次のパラメータを読むために戻る */

	dp->ad=dp->ad+dp->an;			/* アドレスを更新 */
	dp->an=dp->rn;				/* 今回処理した長さを格納 */
	dp->rm=dp->rm-dp->rn;			/* 未処理データを先頭に集める */
	if(dp->rm>0) for(i=0;i<dp->rm;i++) dp->rd[i]=dp->rd[dp->rn+i];
	dp->rn=0;				/* 読み込みデータ読み取り位置を初期化 */
	strcpy(dp->pf,"");			/* 処理中フラグを初期化 */
	dp->lf=po;				/* 改行フラグをセット */
	return(0);
}

/* エラー時の処理 */
cmderr(struct data *dp) {
	int i;

	if(strcmp(dp->pf,"")==0) {
		dp->ad=dp->ad+dp->an;			/* アドレスを更新 */
		dp->an=1;				/* 1バイトだけをエラー表示 */
		dp->vc=0;
		dp->vd[dp->vc++]=dp->rd[dp->rn];	/* その1バイトを数値配列の先頭に格納 */
		dp->rn++;
		dp->rm=dp->rm-dp->rn;			/* 未処理データを先頭に集める */
		if(dp->rm>0) for(i=0;i<dp->rm;i++) dp->rd[i]=dp->rd[dp->rn+i];
		dp->rn=0;				/* 読み込みデータ読み取り位置を初期化 */
		return(0);
	}
	return(-1);
}

int main(int argc,char *argv[]) {
	int i,j,k;
	unsigned long n;
	unsigned char buf[4];
	char s[16384],t[16384],u[16384];
	FILE *fp;
	struct data dp;

	if(argc!=2) {
		printf("usage: a2n seenfile\n");	/* パラメータが1つでなければ使い方表示 */
		return(1);
	}	fp=fopen(argv[1],"rb");				/* SEENファイルを開く */
	if(fp==NULL) {
		printf("%s is not open.\n",argv[1]);	/* 開けなければエラー表示 */
		return(1);
	}

	fseek(fp,0x18,SEEK_SET);			/* シナリオ開始位置データを読み込む */
	fread(buf,1,4,fp);
	n=(buf[0]|buf[1]<<8|buf[2]<<16|buf[3]<<24)*4+0x63;
	fseek(fp,0,SEEK_END);				/* SEENファイルのファイルサイズを求め、 */
	dp.am=ftell(fp)-n;				/* シナリオサイズ（最終アドレス＋１）を求める */

	sscanf(argv[1]+4,"%03d",&k);
	printf("*seen%03d\n",k);

	dp.jc=0;					/* ジャンプテーブル初期化 */
	for(j=0;j<2;j++) {
		fseek(fp,n,SEEK_SET);			/* シナリオ開始位置に移動 */
		dp.pf[0]='\0';				/* パラメータ処理中フラグを初期化 */
		dp.ad=0;				/* コマンドの開始アドレスを初期化 */
		dp.an=0;				/* コマンドのアドレス幅を初期化 */
		dp.rn=0;				/* データバッファ読み込み位置を初期化 */
		dp.rm=0;				/* データバッファ読み込み済みサイズを初期化 */
		while(fread(buf,1,1,fp)!=0) {		/* SEENファイルから1バイト読み込む */
			if(dp.rn>=BUF_SIZE) {		/* バッファ容量を超えたらエラーで強制終了 */
				printf("*** DATA BUFFER OVER FLOW ***\n");
				exit(1);
			}
			dp.rd[dp.rn]=buf[0];		/* データバッファに1バイト追加 */
			dp.rm++;			/* データバッファの大きさを1バイト増やす */
			while(dp.rn<dp.rm) {		/* データバッファが空になるまでループ */
				if(cmdcmp(&dp,"00",0)==0) sprintf(s,"end");
				else if(cmdcmp(&dp,"01",2)==0) sprintf(s,"\\");
				else if(cmdcmp(&dp,"02 03",3)==0) sprintf(s,"br");
				else if(cmdcmp(&dp,"02 01",3)==0) sprintf(s,"br");
				else if(cmdcmp(&dp,"03",1)==0) sprintf(s,"@");
				else if(cmdcmp(&dp,"04 01",0)==0) sprintf(s,"; [04 01] text?");
				else if(cmdcmp(&dp,"0B 01 vv",0)==0) sprintf(s,"; [0B 01] picture? %ld,1",dp.vd[0]);
				else if(cmdcmp(&dp,"0B 02 ss vv vv vv vv vv vv vv vv vv vv vv vv vv vv vv 01",0)==0) sprintf(s,"; [0B 02] bg \"%s\",%s",dp.sd[0],vtos(&dp,t,0,dp.vc,","));
				else if(cmdcmp(&dp,"0B 05 ss vv",0)==0) sprintf(s,"bg \"pdtbmp\\%s.bmp\",1",dp.sd[0]);
				else if(cmdcmp(&dp,"0B 06 ss vv vv vv vv vv vv vv vv vv vv vv vv vv vv vv",0)==0) sprintf(s,"bg \"pdtbmp\\%s.bmp\",1",dp.sd[0]);
				else if(cmdcmp(&dp,"0B 10 ss vv",0)==0) sprintf(s,"bg \"pdtbmp\\%s.bmp\",1",dp.sd[0]);
				else if(cmdcmp(&dp,"0B 22 01 ss 10 01 ss",0)==0) sprintf(s,"bg \"pdtbmp\\%s.bmp\",1\nld c,\":l;pdtbmp\\%s.bmp\",1",dp.sd[0],dp.sd[1]);
				else if(cmdcmp(&dp,"0B 30",0)==0) sprintf(s,"; [0B 30] screen init");
				else if(cmdcmp(&dp,"0E 01 ss",0)==0) sprintf(s,"bgm \"BGM\\%s.WAV\"",dp.sd[0]);
				else if(cmdcmp(&dp,"0E 10 vv",0)==0) sprintf(s,"bgmfadeout %ld\nstop",dp.vd[0]);
				else if(cmdcmp(&dp,"0E 11",0)==0) sprintf(s,"stop");
				else if(cmdcmp(&dp,"0E 21 vv",0)==0) sprintf(s,"dwave 0,\"koewav\\%03ld\\Z%08ld.WAV\"",dp.vd[0]/100000,dp.vd[0]);
				else if(cmdcmp(&dp,"0E 30 ss",0)==0) sprintf(s,"dwave 1,\"DAT\\%s.WAV\"",dp.sd[0]);
				else if(cmdcmp(&dp,"0E 36",0)==0) sprintf(s,"stop");
				else if(cmdcmp(&dp,"13 01 vv",0)==0) sprintf(s,"bg black,1");
				else if(cmdcmp(&dp,"13 02 vv vv",0)==0) sprintf(s,"bg black,1");
				else if(cmdcmp(&dp,"13 10 vv",0)==0) sprintf(s,"bg white,1");
				else if(cmdcmp(&dp,"15 28 37 vv vv 29 aa",0)==0) sprintf(s,"if %%%ld=%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],k,dp.vd[2]);
				else if(cmdcmp(&dp,"15 28 3B vv vv 29 aa",0)==0) sprintf(s,"if %%%ld=%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],k,dp.vd[2]);
				else if(cmdcmp(&dp,"15 28 44 vv vv 26 37 vv vv 29 aa",0)==0) sprintf(s,"if %%%ld>%ld && %%%ld=%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],dp.vd[2],dp.vd[3],k,dp.vd[4]);
				else if(cmdcmp(&dp,"15 28 45 vv vv 29 aa",0)==0) sprintf(s,"if %%%ld<%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],k,dp.vd[2]);
				else if(cmdcmp(&dp,"15 28 46 vv vv 29 aa",0)==0) sprintf(s,"if %%%ld>=%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],k,dp.vd[2]);
				else if(cmdcmp(&dp,"15 28 47 vv vv 29 aa",0)==0) sprintf(s,"if %%%ld<=%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],k,dp.vd[2]);
				else if(cmdcmp(&dp,"15 28 48 vv vv 29 aa",0)==0) sprintf(s,"if %%%ld<>%%%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],k,dp.vd[2]);
				else if(cmdcmp(&dp,"15 28 37 vv vv 26 37 vv vv 26 37 vv vv 26 37 vv vv 26 37 vv vv 26 45 vv vv 29 aa",0)==0) sprintf(s,"if %%%ld=%ld && %%%ld=%ld && %%%ld=%ld && %%%ld=%ld && %%%ld=%ld && %%%ld<%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],dp.vd[2],dp.vd[3],dp.vd[4],dp.vd[5],dp.vd[6],dp.vd[7],dp.vd[8],dp.vd[9],dp.vd[10],dp.vd[11],k,dp.vd[12]);
				else if(cmdcmp(&dp,"15 28 37 vv vv 26 37 vv vv 26 37 vv vv 26 37 vv vv 29 aa",0)==0) sprintf(s,"if %%%ld=%ld && %%%ld=%ld && %%%ld=%ld && %%%ld=%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],dp.vd[2],dp.vd[3],dp.vd[4],dp.vd[5],dp.vd[6],dp.vd[7],k,dp.vd[8]);
				else if(cmdcmp(&dp,"15 28 46 vv vv 26 47 vv vv 26 46 vv vv 26 47 vv vv 29 aa",0)==0) sprintf(s,"if %%%ld>=%ld && %%%ld<=%ld && %%%ld>=%ld && %%%ld<=%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],dp.vd[2],dp.vd[3],dp.vd[4],dp.vd[5],dp.vd[6],dp.vd[7],k,dp.vd[8]);
				else if(cmdcmp(&dp,"15 28 46 vv vv 26 47 vv vv 29 aa",0)==0) sprintf(s,"if %%%ld>=%ld && %%%ld<=%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],dp.vd[2],dp.vd[3],k,dp.vd[4]);
				else if(cmdcmp(&dp,"15 28 3B vv vv 26 37 vv vv 29 aa",0)==0) sprintf(s,"if %%%ld=%ld && %%%ld=%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],dp.vd[2],dp.vd[3],k,dp.vd[4]);
				else if(cmdcmp(&dp,"15 28 28 46 vv vv 29 26 28 47 vv vv 29 29 aa",0)==0) sprintf(s,"if %%%ld>=%ld && %%%ld<=%ld jumpf\ngoto *seen%03d_%ld\n~",dp.vd[0],dp.vd[1],dp.vd[2],dp.vd[3],k,dp.vd[4]);
				else if(cmdcmp(&dp,"16 01 vv",0)==0) sprintf(s,"goto *seen%03ld",dp.vd[0]);
				else if(cmdcmp(&dp,"16 02 vv",0)==0) sprintf(s,"gosub *seen%03ld",dp.vd[0]);
				else if(cmdcmp(&dp,"17 01 vv",0)==0) sprintf(s,"quake 16,%ld",dp.vd[0]);
				else if(cmdcmp(&dp,"19 01 vv",0)==0) sprintf(s,"wait %ld",dp.vd[0]);
				else if(cmdcmp(&dp,"1C aa",0)==0) sprintf(s,"goto *seen%03d_%ld",k,dp.vd[0]);
				else if(cmdcmp(&dp,"1D nn vv (( aa ))",0)==0) {
					for(i=2;i<dp.vc;i++) {
						if(i==2) sprintf(s,"if %%%ld=%d gosub *seen%03d_%ld",dp.vd[1],i-1,k,dp.vd[i]);
						else {
							sprintf(t,"\nif %%%ld=%d gosub *seen%03d_%ld",dp.vd[1],i-1,k,dp.vd[i]);
							strcat(s,t);
						}
					}
				}
				else if(cmdcmp(&dp,"1E nn vv (( aa ))",0)==0) {
					for(i=2;i<dp.vc;i++) {
						if(i==2) sprintf(s,"if %%%ld=%d goto *seen%03d_%ld",dp.vd[1],i-1,k,dp.vd[i]);
						else {
							sprintf(t,"\nif %%%ld=%d goto *seen%03d_%ld",dp.vd[1],i-1,k,dp.vd[i]);
							strcat(s,t);
						}
					}
				}
				else if(cmdcmp(&dp,"20 01",0)==0) sprintf(s,"return");
				else if(cmdcmp(&dp,"20 02",0)==0) sprintf(s,"return");
				else if(cmdcmp(&dp,"37 vv vv",0)==0) sprintf(s,"mov %%%ld,%ld ; [37]",dp.vd[0],dp.vd[1]);
				else if(cmdcmp(&dp,"3B vv vv",0)==0) sprintf(s,"mov %%%ld,%ld",dp.vd[0],dp.vd[1]);
				else if(cmdcmp(&dp,"3C vv vv",0)==0) sprintf(s,"add %%%ld,%ld",dp.vd[0],dp.vd[1]);
				else if(cmdcmp(&dp,"3D vv vv",0)==0) sprintf(s,"sub %%%ld,%ld",dp.vd[0],dp.vd[1]);
				else if(cmdcmp(&dp,"49 vv vv",0)==0) sprintf(s,"mov %%%ld,%%%ld",dp.vd[0],dp.vd[1]);
				else if(cmdcmp(&dp,"58 01 vv 22 00 (( FF ss 00 )) 23",0)==0) sprintf(s,"selnum %%%ld,\"%s\"\nadd %%%ld,1",dp.vd[0],stos(&dp,t,0,dp.sc,"\",\""),dp.vd[0]);
				else if(cmdcmp(&dp,"58 04 vv",0)==0) sprintf(s,"; [58 04] select %ld",dp.vd[0]);
				else if(cmdcmp(&dp,"59 01 11 ss",0)==0) sprintf(s,"; [59 01 11 ] select \"pdtbmp\\%s.bmp\"",dp.sd[0]);
				else if(cmdcmp(&dp,"59 01 12 ss",0)==0) sprintf(s,"; [59 01 12 ] select \"pdtbmp\\%s.bmp\"",dp.sd[0]);
				else if(cmdcmp(&dp,"5B 01 vv (( vv )) 00",0)==0) {
					for(i=1;i<dp.vc;i++) {
						if(i==1) sprintf(s,"mov %%%ld,%ld",dp.vd[0]+i-1,dp.vd[i]);
						else {
							sprintf(t,"\nmov %%%ld,%ld",dp.vd[0]+i-1,dp.vd[i]);
							strcat(s,t);
						}
					}
				}
				else if(cmdcmp(&dp,"5C 01 vv vv vv",0)==0) sprintf(s,"; [5C 01] let %ld,%ld,%ld",dp.vd[0],dp.vd[1],dp.vd[2]);
				else if(cmdcmp(&dp,"5C 02 vv vv vv",0)==0) sprintf(s,"; [5C 02] let %ld,%ld,%ld",dp.vd[0],dp.vd[1],dp.vd[2]);
				else if(cmdcmp(&dp,"60 02 vv",0)==0) sprintf(s,"; [60 02] system %ld",dp.vd[0]);
				else if(cmdcmp(&dp,"60 05",0)==0) sprintf(s,"; [60 05] unknown");
				else if(cmdcmp(&dp,"61 24 nn (( vv FF ss 00 ))",0)==0) {
					for(i=0;i<dp.vd[0];i++) {
						if(i==0) sprintf(s,"; input $%ld,\"%s\",\"$%ld\",6,0",dp.vd[i+1],dp.sd[i],dp.vd[i+1]);
						else {
							sprintf(t,"\n; input $%ld,\"%s\",\"$%ld\",6,0",dp.vd[i+1],dp.sd[i],dp.vd[i+1]);
							strcat(s,t);
						}
					}
				}
				else if(cmdcmp(&dp,"63 01 vv vv vv vv vv",0)==0) sprintf(s,"; [63 01] %%%ld,%%%ld,%%%ld,%%%ld,%ld",dp.vd[0],dp.vd[1],dp.vd[2],dp.vd[3],dp.vd[4]);
				else if(cmdcmp(&dp,"63 02 vv vv vv",0)==0) sprintf(s,"; [63 02] %%%ld,%%%ld,%ld",dp.vd[0],dp.vd[1],dp.vd[2]);
				else if(cmdcmp(&dp,"63 10",0)==0) sprintf(s,"; [63 10] unknown");
				else if(cmdcmp(&dp,"67 01 vv vv vv vv vv vv vv vv vv",0)==0) sprintf(s,"; [67 01] mirror %ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld",dp.vd[0],dp.vd[1],dp.vd[2],dp.vd[3],dp.vd[4],dp.vd[5],dp.vd[6],dp.vd[7],dp.vd[8]);
				else if(cmdcmp(&dp,"67 11 vv vv vv",0)==0) sprintf(s,"; [67 11] screen %ld,%ld,%ld",dp.vd[0],dp.vd[1],dp.vd[2]);
				else if(cmdcmp(&dp,"6D 02 vv vv vv",0)==0) sprintf(s,"clickpos %%%ld,%%%ld\nmov %%%ld,0",dp.vd[0],dp.vd[1],dp.vd[2]);
				else if(cmdcmp(&dp,"6D 03",0)==0) sprintf(s,"; [6D 03] mouse init");
				else if(cmdcmp(&dp,"60 04 FE ss 00",0)==0) sprintf(s,"; [60 04] title [FE],%s",dp.sd[0]);
				else if(cmdcmp(&dp,"60 04 FF ss 00",0)==0) sprintf(s,"; [60 04] title [FF],%s",dp.sd[0]);
				else if(cmdcmp(&dp,"74 02 vv",0)==0) sprintf(s,"; [74 02] unknown %ld",dp.vd[0]);
				else if(cmdcmp(&dp,"FF dd ss",1)==0) sprintf(s,"%s",dp.sd[0]);
				else if(cmderr(&dp)==0) sprintf(s,"; error %02X",dp.vd[0]);
				if(strcmp(dp.pf,"")==0 && j>0) {
					for(i=0;i<dp.jc;i++) if(dp.jd[i]==dp.ad) break;
					if(dp.lc==1 && (dp.lf==0 || i<dp.jc)) printf("/\n");
					if(i<dp.jc) {
						printf("*seen%03d_%ld\n",k,dp.ad);
					}
					if(dp.lc!=1 || dp.lf!=3) printf("%s",s);
					if(dp.lf!=1) printf("\n");
					dp.lc=dp.lf;
				}
			}
		}
	}
	fclose(fp);
	return(0);
}
