# FP_SISOP19_E08

#### Bagas Yanuar Sudrajad - 05111740000074 
#### Octavianus Giovanni Yaunatan - 05111740000113

## Soal

Buatlah sebuah music player dengan bahasa C yang memiliki fitur play nama_lagu, pause, next, prev, list lagu. Selain music player juga terdapat FUSE untuk mengumpulkan semua jenis file yang berekstensi .mp3 kedalam FUSE yang tersebar pada direktori /home/user. Ketika FUSE dijalankan, direktori hasil FUSE hanya berisi file .mp3 tanpa ada direktori lain di dalamnya. Asal file tersebut bisa tersebar dari berbagai folder dan subfolder. program mp3 mengarah ke FUSE untuk memutar musik.

Note: playlist bisa banyak

## Jawaban

Terdapat 2 file, yaitu file readmp3.c dan mediaplayer.c. File readmp3.c untuk membuat FUSE filesystem sedangkan file mediaplayer.c untuk thread menjalankan mp3.

### readmp3.c

Program ini bekerja untuk mendapatkan semua nama file berekstensi mp3 pada folder dan semua sub-folder yang ada.

Secara garis besar program membaca entry suatu folder, jika berupa file dan berekstensi mp3 akan dicatat. Jika merupakan folder maka akan dibaca isinya. Nama-nama file juga disimpan pada struktur data tree untuk disimpan pathnya. Lalu pada implementasinya semua fungsi akan mendapatkan path file dari suatu nama file dari tree tersebut.

```c
#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif


static const char *dirpath = "/home/siung2"; //path root mount

struct node { // untuk tree
    char *value; // nama file
    char *path; // path file
    struct node *p_left;
    struct node *p_right;
    int counter;
};

struct node *root = NULL; //inisialisasi root null
//use typedef to make calling the compare function easier
typedef int (*Compare)(const char *, const char *);

int check(const char* text) ///untuk memeriksa apakah ekstensi mp3
{ 
    int length = strlen(text);
    if(length<5) return 0;
    if(text[length-4]=='.' && text[length-3]=='m' && text[length-2]=='p' && text[length-1]=='3')
        return 1;
    else return 0;
}

//inserts elements into the tree
void insert(char* key, char* path, struct node** leaf, Compare cmp)
{
    int res;
    if( *leaf == NULL ) { // jika tree masih kosong lakukan inisialisasi root
        *leaf = (struct node*) malloc( sizeof( struct node ) );
        (*leaf)->value = malloc( strlen (key) +1 );     // memory untuk nama file
        strcpy ((*leaf)->value, key);                   // copy nama file
        (*leaf)->path = malloc( strlen (path) +1 );     // memory untuk path
        strcpy ((*leaf)->path, path);					// copy path
        (*leaf)->p_left = NULL;
        (*leaf)->p_right = NULL;
        (*leaf)->counter = 0;
    } else {
        res = cmp (key,(*leaf)->value);
        if( res < 0) // ke kiri
            insert( key, path,&(*leaf)->p_left, cmp);
        else if( res > 0) // ke kanan
            insert( key, path,&(*leaf)->p_right, cmp);
        else{ // jika sama
            (*leaf) -> counter++;
            char temp[strlen(key)+100];
            sprintf(temp, "%s#%d", key, (*leaf)->counter);
            printf("%s\n", temp);
            insert(temp,path, &(*leaf)->p_right, cmp);
        }
    }
}

//membandingkan nilai node
int CmpStr(const char *a, const char *b)
{
    return (strcmp (a, b));     //bandingkan nama file
}

//untuk cari path
char* search(char* key, struct node* leaf, Compare cmp)  // no need for **
{
    int res;
    if( leaf != NULL ) { 
        res = cmp(key, leaf->value);
        if( res < 0)
            search( key, leaf->p_left, cmp); //cari ke kiri
        else if( res > 0)
            search( key, leaf->p_right, cmp); // cari ke kanan
        else
            return leaf->path;  // return path
    }
    else return ""; // jika tidak ada
}

void delete_tree(struct node** leaf)
{
    if( *leaf != NULL ) {
        delete_tree(&(*leaf)->p_left);
        delete_tree(&(*leaf)->p_right);
        free( (*leaf)->value );
        free( (*leaf) );
    }
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    char tmp[1024]; 
    strcpy(tmp, path); // copy namafile
    char *fpath = search(tmp, root, CmpStr); // cari path berdasarkan nama file
    int res;

	res = lstat(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int listdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    if (strcmp("/home/siung2/fp/mount", path) == 0) return 0;
    DIR *dp;
	struct dirent *de;
	(void) offset;
	(void) fi;

	dp = opendir(path);
	if (dp == NULL)
		return -errno;
    
	while ((de = readdir(dp)) != NULL) {
        if( !strcmp(de->d_name, ".")|| !strcmp(de->d_name, "..")) //skip . dan ..
            continue;
        if(de->d_type == DT_DIR){ // jika direktori, cari pada folder tersebut
            char new[1024];
            snprintf(new, sizeof(new), "%s/%s", path,de->d_name);
            listdir(new, buf, filler, offset, fi);
        }
        else{// jika bukan direktori
            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;
            char fname[1024], pathFile[1024];
            sprintf(fname, "/%s", de->d_name);
            sprintf(pathFile, "%s/%s", path,de->d_name);
            if(check(de->d_name)){ // jika berekstensi mp3
                if (filler(buf,de->d_name, &st, 0)) //catat untuk ditampilkan
			    break;
                insert(fname, pathFile, &root, CmpStr); //  catat pada tree
            }
        }
	}
	closedir(dp);
	return 0;
}


//fungsi readdir yang pertama dipanggil, lalu memanggil listdir
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	char fpath[1000];
	if(strcmp(path,"/") == 0)
	{
		path=dirpath;
		sprintf(fpath,"%s",path);
	}
	else sprintf(fpath, "%s%s",dirpath,path);
    listdir(fpath, buf, filler, offset,fi);
}

static void *inits(struct fuse_conn_info *conn){
    insert("/",dirpath, &root, CmpStr); // masukkan pada tree untuk root nya
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
    char tmp[1024];
    strcpy(tmp, path);
    char *fpath = search(tmp, root, CmpStr);
    int res;

	res = statvfs(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
    char tmp[1024];
    strcpy(tmp, path);
    char *fpath = search(tmp, root, CmpStr);

	int fd;
	int res;

	(void) fi;
	fd = open(fpath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.readdir	= xmp_readdir,
	.statfs		= xmp_statfs,
    .init       = inits,
    .read       = xmp_read,
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
```



### mediaplayer.c

File ini berfungsi sebagai media player untuk menjalankan file-file mp3 layaknya media player pada umumnya. Di dalamnya terdapat menu-menu untuk menjalankan perintah yang berbeda.

1. help : untuk menampilkan command list yang ada
2. list : untuk menampilkan list lagu yang ada atau menampilkan list lagu dari playlist tertentu
3. play : untuk memainkan lagu tertentu
4. stop : untuk menghentikan lagu yang sedang berjalan
5. next : untuk memainkan lagu berikutnya
6. prev : untuk memainkan lagu sebelumnya
7. pause : untuk memunda lagu yang sedang berjalan
8. resume : untuk melanjutkan lagu yang sebelumnya ditunda
9. playlist : untuk menampilkan list dari playlist yang ada
10. addl : untuk membuat playlist baru
11. reml : untuk menghapus playlist tertentu
12. addsong : untuk menambahkan lagu ke playlist tertentu
13. remsong : untuk menghapus lagu tertentu di sebuah playlist
14. movel : untuk pindah ke sebuah playlist
15. back : untuk kembali ke playlist global
16. exit : untuk keluar dari media player

```c
#include <ao/ao.h>
#include <mpg123.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#define BITS 8

char musicdir[1005] = "/home/ivan/Sisop/FP/Music/";
char song[1005];
int playlist[1005][1005];
char pname[1005][1005];
int totalListSong[1005];
int totalList = 0;
int current_mp3 = -1;
int current_list = -1;
int current_list_mp3 = -1;
int totalsong;
int playstatus = 0;
int stopstatus = 0;
int pausestatus = 0;
int reloadstatus = 0;

pthread_t t;

int isDigit(char* x) {
    for (int i = 0; i < strlen(x); i++) {
        if (!('0' <= x[i] && x[i] <= '9'))
            return 0;
    }
    return 1;
}

void* player(void* arg)
{
    char tmppath[1005];
    mpg123_handle *mh;
    unsigned char *buffer;
    size_t buffer_size;
    size_t done;
    int err;

    int driver;
    ao_device *dev;

    ao_sample_format format;
    int channels, encoding;
    long rate;

    /* initializations */
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = mpg123_outblock(mh);
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));


    /* decode and play */
    while (1) {
        /* open the file and get the decoding format */
        strcpy(tmppath, musicdir);
        DIR *dp;
        struct dirent *de;
        dp = opendir(musicdir);
        int cnt = 0;
        stopstatus = 0;
        while (playstatus == 0)
            sleep(1);
       
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0)
                continue;
            if (cnt == current_mp3) {
                strcat(tmppath, de->d_name);
                break;
            }
            cnt++;
        }
        mpg123_open(mh, tmppath);
        mpg123_getformat(mh, &rate, &channels, &encoding);
        /* set the output format and open the output device */
        format.bits = mpg123_encsize(encoding) * BITS;
        format.rate = rate;
        format.channels = channels;
        format.byte_format = AO_FMT_NATIVE;
        format.matrix = 0;
        dev = ao_open_live(driver, &format, NULL);

        while (playstatus == 1 && stopstatus == 0) {
            while (pausestatus == 1)
                sleep(1);
            if (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
                ao_play(dev, buffer, done);
            else {
                if (current_list == -1)
                    current_mp3 = (current_mp3 + 1 + totalsong) % totalsong;
                else {
                    current_list_mp3 = (current_list_mp3 + 1) % totalListSong[current_list];
                    current_mp3 = playlist[current_list][current_list_mp3];
                }
                break;
            }
        }
    }

    /* clean up */
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();

    return NULL;
}

int main(void)
{
    system("clear");
    memset(totalListSong, 0, sizeof(totalListSong));
    int err = pthread_create(&(t),NULL,&player,NULL); //membuat thread
    DIR *dp;
    struct dirent *de;
    dp = opendir(musicdir);
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0)
            continue;
        totalsong++;
    }
    while (1) {
        char cursong[1005] = "NONE";
        char curplay[1005] = "Global";
        int now = 0;
        dp = opendir(musicdir);
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0)
                continue;
            if (now == current_mp3) {
                strcpy(cursong, de->d_name);
                break;
            }
            now++;
        }
        if (current_list != -1)
            strcpy(curplay, pname[current_list]);

        printf("Current playlist: %s\n", curplay);
        printf("Currently Playing: %s\n", cursong);
        
        char command[1005];
        printf("\nType \"help\" for command list.\n");
        printf("Command: ");
        scanf("%s", command);
        system("clear");
        if (strcmp(command, "stop") == 0) {
            if (playstatus == 1) {
                stopstatus = 1;
                playstatus = 0;
                current_mp3 = -1;
                printf("Song stopped.\n\n");
            }
            else printf("No song is played.\n\n");
        }
        else if (strcmp(command, "help") == 0) {
            printf("Command List:\n");
            printf("1.  list : List all songs\n");
            printf("2.  play ([Song Name].mp3 | [Song Number]) : Play a song\n");
            printf("3.  stop : Stop currently playing song\n");
            printf("4.  next : Play next song\n");
            printf("5.  prev : Play previous song\n");
            printf("6.  pause : Pause current song\n");
            printf("7.  resume : Resume current song\n");
            printf("8.  playlist : List all playlists\n");
            printf("9.  addl [Playlist Name] : Create new playlist\n");
            printf("10. reml [Playlist Name] : Remove playlist\n");
            printf("11. addsong [Playlist Name] press enter, then ([Song Name].mp3 | [Song Number]) : Add song to playlist\n");
            printf("12. remsong [Playlist Name] press enter, then ([Song Name].mp3 | [Song Number]) : Remove song srom playlist\n");
            printf("13. movel [Playlist Name] : Move to playlist\n");
            printf("14. back : Move to Global playlist\n");
            printf("15. help : List all commands\n");
            printf("16. exit : Exit media player\n");
        }
        else if (strcmp(command, "play") == 0) {
            scanf(" %[^\n]", song);
            system("clear");
            dp = opendir(musicdir);
            int zzz = -1;
            int cnt = 0;
            int sz = strlen(song);
            if (sz > 4 && song[sz - 1] == '3' && song[sz - 2] == 'p' && song[sz - 3] == 'm' && song[sz - 4] == '.') {
                while ((de = readdir(dp)) != NULL) {
                    if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0)
                        continue;
                    if (strcmp(de->d_name, song) == 0) {
                        zzz = cnt;
                        break;
                    }
                    cnt++;
                }
            }
            else if (isDigit(song)) {
                int now = atoi(song);
                if (current_list == -1)
                    if (now <= totalsong)
                        zzz = now - 1;
                else {
                    if (now <= totalListSong[current_list])
                        zzz = playlist[current_list][now - 1];
                }
            }
            if (zzz != -1) {
                if (current_list == -1) {
                    stopstatus = 1;
                    playstatus = 1;
                    printf("Playing...\n");
                    current_mp3 = zzz;
                }
                else {
                    int found = 0;
                    for (int i = 0; i < totalListSong[current_list]; i++) {
                        if (playlist[current_list][i] == zzz) {
                            found = 1;
                            current_list_mp3 = i;
                            break;
                        }
                    }
                    if (found) {
                        stopstatus = 1;
                        playstatus = 1;
                        printf("Playing...\n");
                        current_mp3 = zzz;
                    }
                    else printf("Song not available.\n");
                }
            }
            else printf("Song not available.\n");
        }
        else if (strcmp(command, "list") == 0) {
            printf("List Song :\n");
            int cnt = 0;
            if (current_list == -1) {
                dp = opendir(musicdir);
                while ((de = readdir(dp)) != NULL) {
                    if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0)
                        continue;
                    printf("%3d. %s", cnt + 1, de->d_name);
                    if (cnt == current_mp3)
                        printf(" (Now Playing)");
                    printf("\n");
                    cnt++;
                }
            }
            else {
                int localcnt = 0;
                for (int i = 0; i < totalListSong[current_list]; i++) {
                    int cur = playlist[current_list][i];
                    cnt = 0;
                    dp = opendir(musicdir);
                    while ((de = readdir(dp)) != NULL) {
                        if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0)
                            continue;
                        if (cur == cnt) {
                            printf("%3d. %s", localcnt + 1, de->d_name);
                            if (cnt == current_mp3)
                                printf(" (Now Playing)");
                            printf("\n");
                            localcnt++;
                            break;
                        }
                        cnt++;
                    }
                }
            }
        }
        else if (strcmp(command, "prev") == 0) {
            if (current_list == -1)
                current_mp3 = (current_mp3 - 1 + totalsong) % totalsong;
            else {
                current_list_mp3 = (current_list_mp3 - 1 + totalListSong[current_list]) % totalListSong[current_list];
                current_mp3 = playlist[current_list][current_list_mp3];
            }
            stopstatus = 1;
            playstatus = 1;
            printf("Playing Previous Song.\n");
        }
        else if (strcmp(command, "next") == 0) {
            if (current_list == -1)
                current_mp3 = (current_mp3 + 1 + totalsong) % totalsong;
            else {
                current_list_mp3 = (current_list_mp3 + 1) % totalListSong[current_list];
                current_mp3 = playlist[current_list][current_list_mp3];
            }
            stopstatus = 1;
            playstatus = 1;
            printf("Playing Next Song.\n");
        }
        else if (strcmp(command, "pause") == 0) {
            pausestatus = 1;
            if (playstatus == 1) printf("Paused...\n");
            else printf("No song is played.\n");
        }
        else if (strcmp(command, "resume") == 0) {
            pausestatus = 0;
            if (playstatus == 1) printf("Resume...\n");
            else printf("No song is played.\n");
        }
        else if (strcmp(command, "playlist") == 0) {
            if (totalList == 0) printf("No Available playlist.\n");
            else {
                printf("List of playlist\n");
                for (int i = 0; i < totalList; i++)
                    printf("%2d. %s\n", i + 1, pname[i]);
            }
        }
        else if (strcmp(command, "addl") == 0) {
            char tmpplaylist[1005];
            scanf(" %[^\n]", tmpplaylist);
            system("clear");
            int found = 0;
            for (int i = 0; i < totalList; i++) {
                if (strcmp(tmpplaylist, pname[i]) == 0) {
                    found = 1;
                    break;
                }
            }
            if (found) printf("Playlist already exist.\n");
            else {
                strcpy(pname[totalList], tmpplaylist);
                totalList++;
                printf("Playlist Created.\n");
            }
        }
        else if (strcmp(command, "reml") == 0) {
            char tmpplaylist[1005];
            scanf(" %[^\n]", tmpplaylist);
            system("clear");
            int found = -1;
            for (int i = 0; i < totalList; i++) {
                if (strcmp(tmpplaylist, pname[i]) == 0) {
                    found = i;
                    break;
                }
            }
            if (found == -1) printf("Playlist not found.\n");
            else {
                for (int i = found; i < totalList - 1; i++) {
                    strcpy(pname[i], pname[i + 1]);
                    for (int j = 0; j < totalListSong[i + 1]; i++) {
                        playlist[i][j] = playlist[i + 1][j];
                    }
                    totalListSong[i] = totalListSong[i + 1];
                }
                totalList--;
                printf("Playlist Removed.\n");
            }
        }
        else if (strcmp(command, "addsong") == 0) {
            char tmpplaylist[1005];
            scanf(" %[^\n]", tmpplaylist);
            system("clear");
            int plidx = -1;
            for (int i = 0; i < totalList; i++) {
                if (strcmp(tmpplaylist, pname[i]) == 0) {
                    plidx = i;
                    break;
                }
            }
            if (plidx == -1) printf("Playlist not found.\n");
            else {
                char tmpsong[1005];
                dp = opendir(musicdir);
                int cnt = 0;
                while ((de = readdir(dp)) != NULL) {
                    if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0)
                        continue;
                    printf("%d.%s", cnt + 1, de->d_name);
                    if (cnt == current_mp3)
                        printf(" (Now Playing)");
                    printf("\n");
                    cnt++;
                }
                printf("\nChoose Song Name or Song Number:\n");
                scanf(" %[^\n]", tmpsong);
                system("clear");
                dp = opendir(musicdir);
                int sgidx = -1;
                cnt = 0;
                int sz = strlen(tmpsong);
                if (sz > 4 && strcmp(tmpsong+sz-4,".mp3")==0) {
                    while ((de = readdir(dp)) != NULL) {
                        if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0)
                            continue;
                        if (strcmp(de->d_name, tmpsong) == 0) {
                            sgidx = cnt;
                            break;
                        }
                        cnt++;
                    }
                }
                else if (isDigit(tmpsong)) {
                    int now = atoi(tmpsong);
                    if (now <= totalsong) sgidx = now - 1;
                }
                if (sgidx != -1) {
                    int found = 0; 
                    for (int i = 0; i < totalListSong[plidx]; i++) {
                        if (sgidx == playlist[plidx][i]) {
                            found = 1;
                            break;
                        }
                    }
                    if (found) printf("Song already added in the playlist.\n");
                    else {
                        playlist[plidx][totalListSong[plidx]] = sgidx;
                        totalListSong[plidx]++;
                        printf("Song succesfully added to playlist.\n");
                    }
                    
                }
                else printf("Song not available.\n");
            }
        }
        else if (strcmp(command, "remsong") == 0) {
            char tmpplaylist[1005];
            scanf(" %[^\n]", tmpplaylist);
            system("clear");
            int plidx = -1;
            for (int i = 0; i < totalList; i++) {
                if (strcmp(tmpplaylist, pname[i]) == 0) {
                    plidx = i;
                    break;
                }
            }
            if (plidx == -1) printf("Playlist not found.\n");
            else {
                char tmpsong[1005];
                int localcnt = 0;
                int cnt = 0;
                for (int i = 0; i < totalListSong[current_list]; i++) {
                    int cur = playlist[current_list][i];
                    cnt = 0;
                    dp = opendir(musicdir);
                    while ((de = readdir(dp)) != NULL) {
                        if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0)
                            continue;
                        if (cur == cnt) {
                            printf("%d. %s", localcnt + 1, de->d_name);
                            if (cnt == current_mp3)
                                printf(" (Now Playing)");
                            printf("\n");
                            localcnt++;
                            break;
                        }
                        cnt++;
                    }
                }
                printf("\nChoose Song Name or Song Number:\n");
                scanf(" %[^\n]", tmpsong);
                system("clear");
                dp = opendir(musicdir);
                int sgidx = -1;
                cnt = 0;
                int sz = strlen(tmpsong);
                if (sz > 4 && strcmp(tmpsong+sz-4,".mp3")) {
                    int xxx = 0;
                    while ((de = readdir(dp)) != NULL) {
                        if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0)
                            continue;
                        if (strcmp(de->d_name, tmpsong) == 0) {
                            xxx = cnt;
                            break;
                        }
                        cnt++;
                    }
                    for (int i = 0; i < totalListSong[plidx]; i++) {
                        if (playlist[plidx][i] == xxx) {
                            sgidx = i;
                            break;
                        }
                    }
                }
                else if (isDigit(tmpsong)) {
                    int now = atoi(tmpsong);
                    if (now <= totalsong) {
                        sgidx = now - 1;
                    }
                }
                if (sgidx != -1) {
                    for (int i = sgidx; i < totalListSong[plidx] - 1; i++) {
                        playlist[plidx][i] = playlist[plidx][i + 1];
                    }
                    totalListSong[plidx]--;
                    printf("Song removed from the playlist.\n");
                    
                }
                else {
                    printf("Song not available.\n");
                }
            }
        }
        else if (strcmp(command, "movel") == 0) {
            char tmpplaylist[1005];
            scanf(" %[^\n]", tmpplaylist);
            system("clear");
            if (strcmp(pname[current_list], tmpplaylist) == 0) {
                printf("You're already in this playlist.\n");
            }
            else {
                int idx = -1;
                for (int i = 0; i < totalList; i++) {
                    if (strcmp(tmpplaylist, pname[i]) == 0) {
                        idx = i;
                        break;
                    }
                }
                if (idx == -1) {
                    printf("Playlist not found.\n");
                }
                else {
                    playstatus = 0;
                    current_mp3 = -1;
                    current_list = idx;
                    printf("You're moved to the playlist %s.\n",tmpplaylist);
                }
            }
        }
        else if (strcmp(command, "back") == 0) {
            if (current_list == -1) {
                printf("You're already in the global playlist.\n");
            }
            else {
                playstatus = 0;
                current_mp3 = -1;
                current_list = -1;
                printf("Back to the global playlist.\n");
            }
        }
        else if (strcmp(command, "exit") == 0) {
            printf("Closing media player\n");
            sleep(2);
            system("clear");
            exit(0);
        }
        else {
            system("clear");
            printf("Invalid Command.\n");
        }
        printf("\n");
    }
    exit(0);
    return 0;
}
```
