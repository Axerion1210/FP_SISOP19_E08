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