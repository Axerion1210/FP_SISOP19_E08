#include <ao/ao.h>
#include <mpg123.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>

#define BITS 8

pthread_t tid;

char type[100];
int mp3_pause=0;
int mp3_play=0;
int mp3_seek=0; // 0 normal, negative : backward, positive : forward
int select_mp3 = -1;
char now_playing[1024] = "";

char dirpath[] = "/home/ivan/Sisop/FP/Music";

void *player(void *arg){
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

    char* filename = arg;
    char path[1000];
    sprintf(path,"%s/%s",dirpath,filename);

    /* open the file and get the decoding format */
    mpg123_open(mh, path);
    mpg123_getformat(mh, &rate, &channels, &encoding);

    /* set the output format and open the output device */
    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);

    /* decode and play */
    while (1){
        if (mp3_pause==1){
            sleep(1);
            continue;
        }
        if (mp3_play==0) {
            break;
        }

        if (mp3_seek<0) {
            mpg123_seek_frame(mh, mp3_seek, SEEK_CUR);
            mp3_seek = 0;
        }
        else if (mp3_seek>0){
            mpg123_seek_frame(mh, mp3_seek, SEEK_CUR);
            mp3_seek = 0;
        }
        
        
        if(mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
            ao_play(dev, buffer, done);
        else
            break;
        
        //printf("%d %d %d\n", (int) buffer, (int) buffer_size, (int) done);
    }

    /* clean up */
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();

    if (mp3_play==0) {
        return NULL;
    }
    
    select_mp3++;
	DIR *dp;
	struct dirent *de;
    int no = 1;
    dp = opendir(dirpath);
    while ((de = readdir(dp)) != NULL) {
        int len = strlen(de->d_name);
        char fn[1024];
        sprintf(fn, "%s", de->d_name);
        
        if (strcmp(fn+strlen(fn)-4,".mp3")==0) {
            if (no == select_mp3) {
                sprintf(now_playing, "%s", de->d_name);
                no++;
                break;
            }
            no++;
        }
    }
    if (select_mp3<1 || select_mp3>=no){
        select_mp3 = -1;
    }
    else{
        mp3_play = 0;
        sleep(1);
        mp3_play = 1;
        mp3_pause = 0;
        mp3_seek = 0;
        pthread_create(&tid,NULL,&player,now_playing);
    }
    system("clear");
    if (select_mp3 > 0)
        printf("Now Playing : %s\n\n", now_playing);
    
    printf("Help :\nopen to open music\nstop to stop music\n   p to play/pause music\n   , to rewind music\n   . to forward music\nprev to play previous music\nnext to play next music\nexit to exit mp3player\n\n");
    printf("Command : \n");
}

int main(int argc, char *argv[])
{
	DIR *dp;
	struct dirent *de;
    
    system("clear");
    while(1){
        if (select_mp3 > 0)
            printf("Now Playing : %s\n\n", now_playing);
        
        printf("Help :\nopen : open music\nstop : stop music\n   p : play/pause music\n");
        printf("   , : rewind music\n   . : forward music\nprev : play previous music\n");
        printf("next : play next music\nexit : exit media player\n\n");
        printf("Command : ");
        scanf("%s", type);
        system("clear");
        if (strcmp(type,"p")==0) {
            if (mp3_pause)
                mp3_pause = 0;
            else
                mp3_pause = 1;
        }
        else if (strcmp(type,"open")==0){
            printf("PlayList :\n");
            int no = 1;
            dp = opendir(dirpath);
            while ((de = readdir(dp)) != NULL) {
                int len = strlen(de->d_name);
                char fn[1024];
                sprintf(fn, "%s", de->d_name);
                
                if (strcmp(fn+strlen(fn)-4,".mp3")==0) {
                    printf("%3d. %s\n", no++, fn);
                }
            }

            printf("\n\nSong number : ");
            int selected_before = select_mp3;
            scanf("%d", &select_mp3);
            if (select_mp3<1 || select_mp3 >= no) {
                printf("Song not found!\n");
                select_mp3 = selected_before;
                sleep(2);
                system("clear");
                continue;
            }
            

            no=1;
            dp = opendir(dirpath);
            while ((de = readdir(dp)) != NULL) {
                int len = strlen(de->d_name);
                char fn[1024];
                sprintf(fn, "%s", de->d_name);
                
                if (strcmp(fn+strlen(fn)-4,".mp3")==0) {
                    if (no == select_mp3) {
                        sprintf(now_playing, "%s", de->d_name);
                        break;
                    }
                    no++;
                }
            }

            printf("Playing %s\n", now_playing);
            mp3_play = 0;
            sleep(1);
            mp3_play = 1;
            mp3_pause = 0;
            mp3_seek = 0;
            pthread_create(&tid,NULL,&player,now_playing);
            system("clear");
        }
        else if (strcmp(type,"next")==0 || strcmp(type,"prev")==0){
            if(strcmp(type,"next")==0)
                select_mp3++;
            else
                select_mp3--;
            int no = 1;
            dp = opendir(dirpath);
            while ((de = readdir(dp)) != NULL) {
                int len = strlen(de->d_name);
                char fn[1024];
                sprintf(fn, "%s", de->d_name);
                
                if (strcmp(fn+strlen(fn)-4,".mp3")==0) {
                    if (no == select_mp3) {
                        sprintf(now_playing, "%s", de->d_name);
                        no++;
                        break;
                    }
                    no++;
                }
            }
            if (select_mp3 >= no){
                printf("End of song list\n");
                select_mp3--;
                sleep(2);
                system("clear");
                continue;
            }
            else if (select_mp3<1){
                printf("Start of the song list!\n");
                select_mp3++;
                sleep(2);
                system("clear");
                continue;
            }

            printf("Opening %s\n", now_playing);
            mp3_play = 0;
            sleep(1);
            mp3_play = 1;
            mp3_pause = 0;
            mp3_seek = 0;
            pthread_create(&tid,NULL,&player,now_playing);
            system("clear");
        }
        else if (strcmp(type,",")==0)
            mp3_seek -= 200;
        else if (strcmp(type,".")==0)
            mp3_seek += 200;
        else if (strcmp(type,"exit")==0){
            mp3_play = 0;
            printf("Closing media player\n");
            sleep(2);
            system("clear");
            break;
        }
        else if (strcmp(type,"stop")==0){
            mp3_play = 0;
            printf("Media player stopped\n");
            select_mp3 = -1;
            sleep(1);
            system("clear");
        }
        else
        {
            printf("Invalid command!!\n");
            sleep(1);
            system("clear");
        }
        
    }
}
