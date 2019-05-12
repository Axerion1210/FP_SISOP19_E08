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
        while (playstatus == 0) {
            sleep(1);
        }
       
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
            while (pausestatus == 1) {
                sleep(1);
            }
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
        if (current_list != -1) {
            strcpy(curplay, pname[current_list]);
        }

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
            } else {
                printf("No song is played.\n\n");
            }
        } else if (strcmp(command, "help") == 0) {
            printf("Command List:\n");
            printf("1.  list : List all songs\n");
            printf("2.  play ([Song Name].mp3 | [Song Number]) : Play a song\n");
            printf("3.  stop : Stop currently playing song\n");
            printf("4.  next : Play next song\n");
            printf("5.  prev : Play previous song\n");
            printf("6.  pause : Pause current song\n");
            printf("7.  resume : Resume current song\n");
            printf("8.  listp : List all playlists\n");
            printf("9.  addp [Playlist Name] : Make New playlist\n");
            printf("10. remp [Playlist Name] : Remove playlist\n");
            printf("11. addsp [Playlist Name] press enter, then ([Song Name].mp3 | [Song Number]) : Add Song To playlist\n");
            printf("12. remsp [Playlist Name] press enter, then ([Song Name].mp3 | [Song Number]) : Remove Song From playlist\n");
            printf("13. movep [Playlist Name] : Move to playlist\n");
            printf("14. back : Move to Global playlist\n");
            printf("15. help : List all commands\n");
            printf("16. exit : Exit media player\n");
        } else if (strcmp(command, "play") == 0) {
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
            } else if (isDigit(song)) {
                int now = atoi(song);
                if (current_list == -1) {
                    if (now <= totalsong) {
                        zzz = now - 1;
                    }
                } else {
                    if (now <= totalListSong[current_list]) {
                        zzz = playlist[current_list][now - 1];
                    }
                }
            }
            if (zzz != -1) {
                if (current_list == -1) {
                    stopstatus = 1;
                    playstatus = 1;
                    printf("Playing...\n");
                    current_mp3 = zzz;
                } else {
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
                    } else {
                        printf("Song not available.\n");
                    }
                }
            } else {
                printf("Song not available.\n");
            }
        } else if (strcmp(command, "list") == 0) {
            system("clear");
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
            } else {
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
        } else if (strcmp(command, "pause") == 0) {
            system("clear");
            pausestatus = 1;
            if (playstatus == 1) {
                printf("Paused...\n");
            } else {
                printf("No song is played.\n");
            }
        } else if (strcmp(command, "prev") == 0) {
            system("clear");
            if (current_list == -1)
                current_mp3 = (current_mp3 - 1 + totalsong) % totalsong;
            else {
                current_list_mp3 = (current_list_mp3 - 1 + totalListSong[current_list]) % totalListSong[current_list];
                current_mp3 = playlist[current_list][current_list_mp3];
            }
            stopstatus = 1;
            playstatus = 1;
            printf("Playing Previous Song.\n");
        } else if (strcmp(command, "next") == 0) {
            system("clear");
            if (current_list == -1)
                current_mp3 = (current_mp3 + 1 + totalsong) % totalsong;
            else {
                current_list_mp3 = (current_list_mp3 + 1) % totalListSong[current_list];
                current_mp3 = playlist[current_list][current_list_mp3];
            }
            stopstatus = 1;
            playstatus = 1;
            printf("Playing Next Song.\n");
        } else if (strcmp(command, "resume") == 0) {
            system("clear");
            pausestatus = 0;
            if (playstatus == 1) {
                printf("Resume...\n");
            } else {
                printf("No song is played.\n");
            }
        } else if (strcmp(command, "listp") == 0) {
            system("clear");
            if (totalList == 0) {
                printf("No Available playlist.\n");
            } else {
                printf("List of playlist\n");
                for (int i = 0; i < totalList; i++) {
                    printf("%2d. %s\n", i + 1, pname[i]);
                }
            }
        } else if (strcmp(command, "addp") == 0) {
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
            if (found) {
                printf("Playlist already exist.\n");
            } else {
                strcpy(pname[totalList], tmpplaylist);
                totalList++;
                printf("Playlist Created.\n");
            }
        } else if (strcmp(command, "remp") == 0) {
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
            if (found == -1) {
                printf("Playlist not found.\n");
            } else {
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
        } else if (strcmp(command, "addsp") == 0) {
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
            if (plidx == -1) {
                printf("Playlist not found.\n");
            } else {
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
                } else if (isDigit(tmpsong)) {
                    int now = atoi(tmpsong);
                    if (now <= totalsong) {
                        sgidx = now - 1;
                    }
                }
                if (sgidx != -1) {
                    int found = 0; 
                    for (int i = 0; i < totalListSong[plidx]; i++) {
                        if (sgidx == playlist[plidx][i]) {
                            found = 1;
                            break;
                        }
                    }
                    if (found) {
                        printf("Song already added in the playlist.\n");
                    } else {
                        playlist[plidx][totalListSong[plidx]] = sgidx;
                        totalListSong[plidx]++;
                        printf("Song succesfully added to playlist.\n");
                    }
                    
                } else {
                    printf("Song not available.\n");
                }
            }
        } else if (strcmp(command, "remsp") == 0) {
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
            if (plidx == -1) {
                printf("Playlist not found.\n");
            } else {
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
                } else if (isDigit(tmpsong)) {
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
                    
                } else {
                    printf("Song not available.\n");
                }
            }
        } else if (strcmp(command, "movep") == 0) {
            char tmpplaylist[1005];
            scanf(" %[^\n]", tmpplaylist);
            system("clear");
            if (strcmp(pname[current_list], tmpplaylist) == 0) {
                printf("You're already in this playlist.\n");
            } else {
                int idx = -1;
                for (int i = 0; i < totalList; i++) {
                    if (strcmp(tmpplaylist, pname[i]) == 0) {
                        idx = i;
                        break;
                    }
                }
                if (idx == -1) {
                    printf("Playlist not found.\n");
                } else {
                    playstatus = 0;
                    current_mp3 = -1;
                    current_list = idx;
                    printf("You're moved to the playlist %s.\n",tmpplaylist);
                }
            }
        } else if (strcmp(command, "back") == 0) {
            system("clear");
            if (current_list == -1) {
                printf("You're already in the global playlist.\n");
            } else {
                playstatus = 0;
                current_mp3 = -1;
                current_list = -1;
                printf("Back to the global playlist.\n");
            }
        } else if (strcmp(command, "exit") == 0) {
            printf("Closing media player\n");
            sleep(2);
            system("clear");
            exit(0);
        } else {
            system("clear");
            printf("Invalid Command.\n");
        }
        printf("\n");
    }
    exit(0);
    return 0;
}