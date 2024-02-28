#include <dirent.h> //ls EC
#include <errno.h>
#include <grp.h>
#include <linux/limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> //ls EC
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h> //stat EC
#include <unistd.h>

// colors
#define BLUE "\x1b[34;1m"
#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"
#define HGREEN "\e[0;92m"
#define BBLUE "\e[1;94m"
#define HBRED "\e[0;101m"
#define DEFAULT "\x1b[0m"

volatile sig_atomic_t gSignalStatus = 0; // signal status
int chris = 0; // gobal variable for if in child

// Function that when recieving sigint will set the signal status to 1 and maybe return a newline if in child
void sig_handler(int sig)
{
    gSignalStatus = 1;
    if (chris == 1) {
        fprintf(stdout, "\n");
    }
}

// Function to print the file permissions of a file given to it in the letter format
void do_perm(struct stat file)
{
    fprintf(stdout, (S_ISDIR(file.st_mode)) ? "%sd" : "%s-", DEFAULT); // print out in the default color
    fprintf(stdout, (file.st_mode & S_IRUSR) ? "r" : "-"); // use the file stat commands with the S_I family of functions to get such data
    fprintf(stdout, (file.st_mode & S_IWUSR) ? "w" : "-");
    fprintf(stdout, (file.st_mode & S_IXUSR) ? "x" : "-");
    fprintf(stdout, (file.st_mode & S_IRGRP) ? "r" : "-");
    fprintf(stdout, (file.st_mode & S_IWGRP) ? "w" : "-");
    fprintf(stdout, (file.st_mode & S_IXGRP) ? "x" : "-");
    fprintf(stdout, (file.st_mode & S_IROTH) ? "r" : "-");
    fprintf(stdout, (file.st_mode & S_IWOTH) ? "w" : "-");
    fprintf(stdout, (file.st_mode & S_IXOTH) ? "x" : "-");
}

// Based on a file given will printout the name of it in its proper color according to linux standards
void do_colors(char* name, struct stat file, int l)
{
    switch (file.st_mode & S_IFMT) { // switch statement to choose which file we are currently examining using S_IFMT
    case S_IFREG:
        if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip") || strstr(name, ".rpm")) {
            fprintf(stdout, "%s%s\n", RED, name);
        } else if (strstr(name, ".jpg") || strstr(name, ".gif") || strstr(name, ".bmp") || strstr(name, ".png") || strstr(name, ".tif")) {
            fprintf(stdout, "%s%s\n", MAG, name);
        } else if (file.st_mode & S_IXUSR) {
            if (l == 1) {
                fprintf(stdout, "%s%s%s*\n", GRN, name, DEFAULT);
            } else {
                fprintf(stdout, "%s%s%s\n", GRN, name, DEFAULT);
            }
        } else {
            fprintf(stdout, "%s%s\n", DEFAULT, name);
        }
        break;
    case S_IFDIR:
        if (l == 1) {
            fprintf(stdout, "%s%s/\n", BLUE, name);
        } else {
            fprintf(stdout, "%s%s\n", BLUE, name);
        }
        break;
    case S_IFLNK:
        fprintf(stdout, "%s%s\n", CYN, name);
        break;
    default:
        fprintf(stdout, "%s%s\n", DEFAULT, name);
        break;
    }
}
/*function to help exectute ls and ll*/
void help_ls(int t, int l, struct stat file, char* name)
{
    struct passwd* pw; // used for passwd
    struct group* gr; // used for group
    char* c_time_string;
    char time_string[26];
    if (t == 1) { // only if not getting total blocks we do this
        if (l == 1) { // for ll command we basically just use stat command to get same format as the ll command
            pw = getpwuid(file.st_uid);
            gr = getgrgid(file.st_gid);
            c_time_string = ctime(&file.st_mtime);
            snprintf(time_string, sizeof(time_string), "%.*s", (int)strlen(c_time_string) - 1, c_time_string);
            do_perm(file);
            fprintf(stdout, " ");
            fprintf(stdout, "%ld ", file.st_nlink);
            fprintf(stdout, "%s ", pw->pw_name);
            fprintf(stdout, "%s ", gr->gr_name);
            fprintf(stdout, "%lld ", (long long)file.st_size);
            fprintf(stdout, "%s ", time_string);
        }
        do_colors(name, file, l);
    }
}
/*function to exectute ls and ll
CurrD: Directory we wish to list
file: struct for getting info from files
i: used to count how many arguments we have done for this command
l: used to indicate if are running ll
t: used to if we are counting total blocks
r: used to count how many iterations we are on
totalL: used for counting total block size in ll*/
void do_ls(char* currD, struct stat file, int i, int l, int t, long long* totalL, int r)
{
    if (currD != NULL) { // check if directory was null and get stat
        if (stat(currD, &file) == -1) {
            fprintf(stderr, "%sError: Cannot get file stat for %s. %s.%s\n", RED, currD, strerror(errno), DEFAULT);
            return;
        }
        if (!S_ISDIR(file.st_mode)) { // check if directory if so do the helper command
            help_ls(t, l, file, currD);
            return;
        }
    }
    if (*totalL == 0 && l == 1 && t == 1) { // if we are running ll and total is 0 then we run this iteration to get the total
        do_ls(currD, file, i, l, 0, totalL, r);
    }
    char path[PATH_MAX];
    DIR* dir;
    struct dirent* curr;
    char* cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        fprintf(stderr, "%sError: Cannot get current working directory. %s.%s\n", RED, strerror(errno), DEFAULT);
    }
    if (i == 1) { // if 1 arg was supplied we just use the home directory
        dir = opendir(".");
        if (dir == NULL) {
            fprintf(stderr, "%sError: Cannot open directory %s. %s.%s\n", RED, cwd, strerror(errno), DEFAULT);
        }
    } else { // overwise we use what was supplied to currD
        dir = opendir(currD);
        if (dir == NULL) {
            fprintf(stderr, "%sError: Cannot open directory %s. %s.%s\n", RED, cwd, strerror(errno), DEFAULT);
        }
    }
    if (dir != NULL) { // if the directory is real we run this
        if ((i > 2 && l == 0) || (i > 2 && l == 1 && t == 0)) {
            if (r > 1) { // if we are on another iteration we print a line to space out
                fprintf(stdout, "\n");
            }
            fprintf(stdout, "%s%s:\n", DEFAULT, currD); // print the current directory
        }
        if (t == 1 && l == 1) { // print total blocksize
            fprintf(stdout, "%stotal %lld\n", DEFAULT, *totalL);
        }
        while ((curr = readdir(dir)) != NULL) { // go through everyfile and return the name using stat and the helper
            if (((curr->d_name)[0] != '.' && l == 0) || l == 1) {
                snprintf(path, sizeof(path), "%s/%s", currD, curr->d_name);
                if (i > 1) {
                    if (stat(path, &file) == -1) {
                        fprintf(stderr, "%sError: Cannot get file stat for %s. %s.%s\n", RED, curr->d_name, strerror(errno), DEFAULT);
                        break;
                    }
                } else {
                    if (stat(curr->d_name, &file) == -1) {
                        fprintf(stderr, "%sError: Cannot get file stat for %s. %s.%s\n", RED, curr->d_name, strerror(errno), DEFAULT);
                        break;
                    }
                }
                if (t == 0) { // if checking for total blocks we use this
                    (*totalL) = (*totalL) + (file.st_blocks / 2);
                }
                help_ls(t, l, file, curr->d_name);
            }
        }
    }
    free(cwd); // free
    closedir(dir); // close
}

/*Function to use for find is called samePerms cause I just copypasted most of this from hw3*/
void samePerms(char* path, char* name, char* og_path, int isDir)
{
    DIR* dp;
    struct dirent* dirp;
    struct stat currFile;
    int dir = 0;
    if (name != NULL) {
        stat(name, &currFile);
        dir = S_ISDIR(currFile.st_mode);
    }
    if (dir) { // if its directory recursively call
        dp = opendir(name);
        if (chdir(name) != 0) { // error check the directory
            fprintf(stderr, "%sError: Cannot open directory '%s'. %s.%s\n", RED, path, strerror(errno), DEFAULT);
            if (strcmp(og_path, path) == 0) {
                closedir(dp);
                return;
            } else {
                closedir(dp);
                return;
            }
        }
        fprintf(stdout, "%s\n", name); // print out file if found
        isDir = 1;
    } else {
        dp = opendir(path);
        if (chdir(path) != 0) { // error check the directory
            fprintf(stderr, "%sError: Cannot open directory '%s'. %s.%s\n", RED, path, strerror(errno), DEFAULT);
            if (strcmp(og_path, path) == 0) {
                closedir(dp);
                return;
            } else {
                closedir(dp);
                return;
            }
        }
        if (name == NULL) { /*checks if the argument is a directory in the case where we are asked to print everything out*/
            isDir = 1;
        }
    }
    int valid = 1; // set valid bit in order for printing
    while ((dirp = readdir(dp)) != NULL) { // loop through directory
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0 || strcmp(dirp->d_name, "snap") == 0) {
        } else {
            stat(dirp->d_name, &currFile); // if not . or .. folders we get stat of the current file
            if (S_ISDIR(currFile.st_mode) != 0) { // if its directory recursively call
                if(isDir == 1){
                    fprintf(stdout, "%s/%s\n", path, dirp->d_name); // print out file if found
                    samePerms(dirp->d_name, NULL, name, isDir);
                }
                else{
                    samePerms(dirp->d_name, name, og_path, isDir);
                }
            } else {
                if(isDir == 0){
                    if (strcmp(dirp->d_name, name) != 0) { // if the current file isnt the one we are searching for set valid to 0
                        valid = 0;
                    }
                }
                if (valid == 1 || isDir == 1) {
                    char* path = getcwd(NULL, 0);
                    fprintf(stdout, "%s/%s\n", path, dirp->d_name); // print out file if found
                    free(path);
                }
                valid = 1;
            }
        }
    }
    if (strcmp(og_path, path) != 0) { // if not in original directory go back
        chdir("..");
    }
    closedir(dp); // close
}

/*Function to do the stat function call*/
void do_stat(char* filename)
{
    struct stat file_stat;
    if (stat(filename, &file_stat) < 0) { // get stat of the file supplied
        fprintf(stderr, "%sError: Cannot get stat of file. %s.%s\n", RED, strerror(errno), DEFAULT);
        return;
    }
    struct passwd* pw = getpwuid(file_stat.st_uid); // get passwd
    struct group* gr = getgrgid(file_stat.st_gid); // get group
    fprintf(stdout, "File: %s\n", filename); // this is kind of the same as ll but in the formating for stat so all the same with the stat struct
    fprintf(stdout, "Size: %lld\n", (long long)file_stat.st_size);
    fprintf(stdout, "Blocks: %lld\n", (long long)file_stat.st_blocks);
    fprintf(stdout, "I/O Block: %ld\n", file_stat.st_blksize);
    switch (file_stat.st_mode & S_IFMT) {
    case S_IFBLK:
        fprintf(stdout, "block device\n");
        break;
    case S_IFCHR:
        fprintf(stdout, "character device\n");
        break;
    case S_IFDIR:
        fprintf(stdout, "directory\n");
        break;
    case S_IFIFO:
        fprintf(stdout, "FIFO/pipe\n");
        break;
    case S_IFLNK:
        fprintf(stdout, "symlink\n");
        break;
    case S_IFREG:
        if (file_stat.st_size == 0) {
            fprintf(stdout, "regular empty file\n");
        } else {
            fprintf(stdout, "regular file\n");
        }
        break;
    case S_IFSOCK:
        fprintf(stdout, "socket\n");
        break;
    default:
        fprintf(stdout, "?\n");
        break;
    }
    fprintf(stdout, "Device: %lxh/%lud\n", file_stat.st_dev, file_stat.st_dev);
    fprintf(stdout, "Inode: %ld\n", file_stat.st_ino);
    fprintf(stdout, "Links: %ld\n", file_stat.st_nlink);
    fprintf(stdout, "Access: (%o/", file_stat.st_mode & 0777);
    do_perm(file_stat);
    fprintf(stdout, ")\n");
    fprintf(stdout, "UID: (%d/%s)\n", file_stat.st_uid, pw->pw_name);
    fprintf(stdout, "GID: (%d/%s)\n", file_stat.st_gid, gr->gr_name);
    fprintf(stdout, "Access: %s", ctime(&file_stat.st_atime));
    fprintf(stdout, "Modify: %s", ctime(&file_stat.st_mtime));
    fprintf(stdout, "Change: %s", ctime(&file_stat.st_ctime));
    fprintf(stdout, "Birth: -\n"); // many UNIX systems dont have birth so we actually cannot accomplish this sadly
}

/*Helper function to get correct printout for tree*/
int files_count(char* path, char* og_path)
{
    DIR* dp2;
    struct dirent* dirp;
    dp2 = opendir(path);
    if (dp2 == NULL) {
        closedir(dp2);
        return 0;
    }
    int entries = 0;
    struct stat currFile;
    while ((dirp = readdir(dp2)) != NULL) { // iterate througn the directory and count how many files are in it not counting hidden ones
        if ((dirp->d_name[0]) != '.') {
            stat(dirp->d_name, &currFile);
            if (S_ISDIR(currFile.st_mode) != 0) { // is able to recursively call
                if (chdir(dirp->d_name) == 0) {
                    chdir("..");
                    entries++;
                }
            } else {
                entries++;
            }
        }
    }
    closedir(dp2);
    return entries; // return the amount given
}

/*Function to preform the tree command
path: path of current dir we are working with
og_path: path of the base of the tree
files: total num of files in the tree
dirs: total num of dirs in the tree
prev_files: amount of files from prev call
is very similar to samePerms*/
void do_tree(char* path, char* og_path, int iteration, int* files, int* dirs, int prev_files)
{
    DIR* dp;
    struct dirent* dirp;
    struct stat currFile;
    int files_left = files_count(path, og_path); // get the total num of files in the directory
    dp = opendir(path); // try to open the path
    if (chdir(path) != 0) {
        if (strcmp(og_path, path) == 0) {
            fprintf(stderr, "%sError: Cannot get directoy. %s.%s\n", RED, strerror(errno), DEFAULT);
            closedir(dp);
            return;
        } else {
            closedir(dp);
            return;
        }
    } else {
        for (int i = 0; i < (iteration - 1); i++) { // formatting stuff for the print out to get it on the right layer
            if (i < (iteration - 2)) {
                fprintf(stdout, "│");
            } else {
                if (prev_files == 1) {
                    fprintf(stdout, "└");
                } else {
                    fprintf(stdout, "├");
                }
            }
            if ((i + 1) != (iteration - 1)) {
                fprintf(stdout, "  ");
            }
        }
        if (iteration > 1) {
            fprintf(stdout, "──");
        }
        fprintf(stdout, "%s%s%s\n", BLUE, path, DEFAULT); // printout the directory
    }
    while ((dirp = readdir(dp)) != NULL) { // iterate through all files
        if ((dirp->d_name[0]) == '.') { // if its not a hidden file
        } else {
            stat(dirp->d_name, &currFile); // get stat
            if (S_ISDIR(currFile.st_mode) != 0) {
                (*dirs) += 1; // increase total dirs by 1
                do_tree(dirp->d_name, og_path, iteration + 1, files, dirs, files_left); // call recurisively on this directory
            } else {
                for (int i = 0; i < iteration; i++) { // formatting stuff once more
                    if (i + 1 == iteration) {
                        if (files_left <= 1) {
                            fprintf(stdout, "└");
                        } else {
                            fprintf(stdout, "├");
                        }
                    } else { // more format
                        if (prev_files == 1 && (i + 2 == iteration)) {
                            fprintf(stdout, " ");
                        } else {
                            fprintf(stdout, "│");
                        }
                    }
                    if ((i + 1) != iteration) { // more foramt
                        fprintf(stdout, "  ");
                    }
                }
                fprintf(stdout, "──");
                (*files) += 1; // increase the file count by 1
                do_colors(dirp->d_name, currFile, 0); // we printout the current file using our color function
            }
            files_left--; // successful iteration so decrease current files by 1
        }
    }
    if (strcmp(og_path, path) != 0) { // if not in original directory go back
        chdir("..");
    }
    closedir(dp); // close
}

/*Function used in order to find if the l flag is in ls so it can do its proper output as I did both ls and ll ec which are a bit different so it can just
execute based on execvp*/
int find_l_flag(char** args)
{
    int i = 0;
    while (args[i] != NULL) { // go through all the arguments
        if (strcmp(args[i], "-l") == 0) { // if the current string is -l then we return 1 for true
            return 1;
        }
        i++;
    }
    return 0; // if not 0 for false
}

/*Function to evoke the execvp for different commands*/
int do_exec(int status, char** token)
{
    pid_t pid;
    pid = fork(); // get the process id of the fork command
    chris = 1; // going into child so we make our global variable 1
    if (pid < 0) { // if pid is less than 1 than the fork failed
        fprintf(stderr, "%sError: fork() failed. %s.\n", RED, strerror(errno));
    } else if (pid == 0) { // if the pid is 0 than the its the child
        if (execvp(token[0], token) == -1) { // call execvp for the child based on the arguments supplied
            fprintf(stderr, "%sError: exec() failed. %s.\n", RED, strerror(errno));
            return -1;
        }
    } else {
        sigset_t mask, oldmask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigprocmask(SIG_BLOCK, &mask, &oldmask);
        if (waitpid(pid, &status, 0) == -1) { // wait for child
            if (gSignalStatus) { // if it was interuptted
            } else { // actual error
                fprintf(stderr, "%sError: wait() failed. %s.\n", RED, strerror(errno));
            }
        }
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
    }
    chris = 0; // make global variable 0
    return 0;
}

int main()
{
    char input[PATH_MAX]; // string for our input
    char* cwd;
    char* token[PATH_MAX]; // string array for all the arguments why did I make it path_max kinda funny though
    uid_t uid;
    int status = 0;
    int i, j;
    struct passwd* pw;
    struct stat file;
    int files = 0; // files, dirs, and totalL all used for function calls
    int dirs = 0;
    long long totalL = 0;
    while (1) {
        if (gSignalStatus) { // if interupted we continue
            gSignalStatus = 0;
            continue;
        }
        cwd = getcwd(NULL, 0); // get our cwd
        if (cwd == NULL) { // if failed then error
            fprintf(stderr, "%sError: Cannot get current working directory. %s.\n", RED, strerror(errno));
            return -1;
        }
        fprintf(stdout, "%s[%s]%s> ", BBLUE, cwd, DEFAULT); // print out our prompt
        struct sigaction sa; // declare our sighandlers
        struct sigaction old;
        memset(&sa, 0, sizeof(struct sigaction)); // wipe the sighdnler
        sa.sa_handler = sig_handler; // put the function onto the sighandler
        if (sigaction(SIGINT, &sa, &old) != 0) { // install and print error if issue
            fprintf(stderr, "%sError: Cannot register signal handler. %s.\n", RED, strerror(errno));
            free(cwd);
            return -1;
        }
        i = 0;
        if (fgets(input, PATH_MAX, stdin) == NULL) { // get input and check if its null
            if (gSignalStatus) { // if actually was just interupted just continue to next
                free(cwd);
                gSignalStatus = 0;
                fprintf(stdout, "\n");
                continue;
            }
            fprintf(stderr, "%sError: Failed to read from stdin. %s.\n ", RED, strerror(errno)); // otherwise then error
        }
        if (strcmp(input, "\n") == 0) { // if the input was just error we go again
            free(cwd);
            continue;
        }
        token[i] = strtok(input, " \n"); // get initial arugment for the shell
        while (token[i] != NULL) { // continue through the entire string to get all the arguments with the use of strtok
            i++;
            token[i] = strtok(NULL, " \n");
        }
        if (strcmp(token[0], "exit") == 0) { // if exit then break out and return
            break;
        } else if (strcmp(token[0], "cd") == 0) { // if the argument was cd do this
            if (i > 2) {
                fprintf(stderr, "%sError: Too many arguments to cd.\n", RED); // if more than 2 arguments then an error
            } else if (i == 1 || strcmp(token[1], "~") == 0) { // if only 1 argument or 2nd arg ~ then go to the user home directory
                uid = getuid();
                pw = getpwuid(uid);
                if (pw == NULL) { // if passwd is null then we have an error
                    fprintf(stderr, "%sError: Cannot get passwd entry. %s.\n", RED, strerror(errno));
                } else {
                    if (chdir(pw->pw_dir)) { // try to change to new directory
                        fprintf(stderr, "%sError: Cannot change directory to '%s'. %s.\n", RED, pw->pw_dir, strerror(errno));
                    }
                }
            } else {
                if (chdir(token[1])) { // try to change to supplied directory
                    fprintf(stderr, "%sError: Cannot change directory to '%s'. %s.\n", RED, token[1], strerror(errno));
                }
            }
        } else if (strcmp(token[0], "ls") == 0) { // if ls then execture this
            if (!find_l_flag(token)) { // if no l flag found then do this
                if (i > 2) { // if more than 1 argumen we use j and a while to keep calling said function
                    j = 1;
                    while (token[j] != NULL) {
                        do_ls(token[j], file, i, 0, 1, &totalL, j);
                        j++;
                    }
                } else { // otherwise call once on supplied arguments
                    do_ls(token[1], file, i, 0, 1, &totalL, 0);
                }
            } else { // otherwise execute ls -l
                if (do_exec(status, token) == -1) {
                    break;
                }
            }
        } else if (strcmp(token[0], "find") == 0) {
            if (i > 3) {
                fprintf(stderr, "%sError: Too many arguments to find.\n", RED);
            } else if (i < 2) {
                samePerms(cwd, ".", cwd, 0);
                chdir(cwd); // change back to cwd
            } else {
                if (i == 3) { // if 3 arguments supplied then we call find on samePerms with the directory as 2nd input and file as 3rd
                    samePerms(token[1], token[2], token[1], 0);
                    chdir(cwd); // change back to cwd
                } else { // if only 1 input then its just the file and we call find only on 1 argument under the cwd
                    samePerms(cwd, token[1], cwd, 0);
                    chdir(cwd); // change back to cwd
                }
            }
        } else if (strcmp(token[0], "stat") == 0) { // do this if stat as arg
            if (i < 2) { // if just 1 arg than an issue
                fprintf(stderr, "%sError: Too few arguments for stat.%s\n", RED, DEFAULT);
            } else {
                j = 1; // same concept as ls where we keep calling ls but now with stat on all the files add newline in between them
                while (token[j] != NULL) {
                    if (j != 1) {
                        fprintf(stdout, "\n");
                    }
                    do_stat(token[j]);
                    j++;
                }
            }
        } else if (strcmp(token[0], "ll") == 0) { // basically functions the exact same as ls but now we are we change the l input in order to do ll instead
            if (i > 2) {
                j = 1;
                while (token[j] != NULL) {
                    do_ls(token[j], file, i, 1, 1, &totalL, j);
                    totalL = 0;
                    j++;
                }
            } else {
                do_ls(token[1], file, i, 1, 1, &totalL, 0);
                totalL = 0; // reset this variable
            }
        } else if (strcmp(token[0], "tree") == 0) {
            if (i == 1) { // if only 1 argument do it on the cwd
                do_tree(".", ".", 1, &files, &dirs, 0);
            } else { // same concept once more as ls but now for tree
                j = 1;
                while (token[j] != NULL) {
                    do_tree(token[j], token[j], 1, &files, &dirs, 0);
                    j++;
                }
                chdir(cwd);
            }
            fprintf(stdout, "\n"); // if statements for all possibles printouts grammar
            if (dirs == 1) {
                fprintf(stdout, "%d directory, ", dirs);
            } else {
                fprintf(stdout, "%d directories, ", dirs);
            }
            if (files == 1) {
                fprintf(stdout, "%d file\n", files);
            } else {
                fprintf(stdout, "%d files\n", files);
            }
            files = 0; // reset these varaibles
            dirs = 0;
        } else {
            if (do_exec(status, token) == -1) {
                break;
            } // otherwise we call execvp
        }
        free(cwd); // free cwd
    }
    free(cwd); // free cwd
    return EXIT_SUCCESS; // exit success
}