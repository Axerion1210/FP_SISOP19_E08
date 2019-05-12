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

  ```c
  ```

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
