#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

/*
The function of the program is to analyze a file for the largest word using multiple threads.
this is done by segmenting the file into "chunks", which each is being analyzed by a thread.
*/

int thread_char_length = 500;
int offset_total;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*holds the information for a thread*/
struct thread_data
{
    int thread_id;
    FILE *file;
    int result_word_size;
    int result_word_count;
    int offset_thread_end;
    int offset_thread_total;
};

/*checks whether or not a character is a seperator*/
int checkForSeperator(char ch)
{
    if (ch == ' ' || ch == '\t' || ch == '\n')
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void *checkForLongestWord(void *arg)
{
    struct thread_data *tdata = (struct thread_data *)arg;
    FILE *f = tdata->file;
    /*stores the chunk of the file which the thread is responsible for*/
    char ch[thread_char_length + tdata->offset_thread_end];
    /*finds the point in the file the thread is responsible for and stores it in ch*/
    pthread_mutex_lock(&mutex);
    fseek(f, ((tdata->thread_id * thread_char_length) + tdata->offset_thread_total), SEEK_SET); //increasing the value until the end of the word
    fread(ch, 1, thread_char_length + tdata->offset_thread_end, f);
    pthread_mutex_unlock(&mutex);
    
    int word_size;
    int max_word_size;
    int word_count;
    int j;
    /*goes through each character and checks if it's a seperator to find the largest word*/
    do
    {
        if (checkForSeperator(ch[j]) == 1)
        {
            word_size = 0;
            word_count++;
        }
        else
        {
            word_size++;
        }
        if (word_size > max_word_size)
        {
            max_word_size = word_size;
        }
        j++;
    } while (ch[j] != '\0' && j < thread_char_length + tdata->offset_thread_end);
    tdata->result_word_size = max_word_size;
    tdata->result_word_count = word_count;
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    /*Finds file to analyze*/
    char file_path[100];
    printf("%s", "Enter source filepath \n");
    scanf("%s", file_path);
    FILE *file;

    /*verifies if the file exists*/
    file = fopen(file_path, "r");
    if (file == NULL)
    {
        perror("Failed to open file");
        exit(0);
    }
    /*Gets the file size and uses this value to determine how many threads should be created*/
    long f_d = fileno(file);
    struct stat buffer;
    fstat(f_d, &buffer);
    long file_length = 0;
    file_length = buffer.st_size;
    int no_of_threads = (int)ceil((double)file_length / (double)thread_char_length);


    printf("%s %d", "Number of threads created:", no_of_threads);
    struct thread_data thread_data_array[no_of_threads];
    pthread_t threads[no_of_threads];

    for (int i = 0; i < no_of_threads; i++)
    {
        /*creates a struct with information needed by each thread*/
        thread_data_array[i].thread_id = i;
        thread_data_array[i].file = file;

        /*if a word would normally be seperated between 2 threads, then this part of the code
        extends the size of the chunk of the file the thread is responsible for analyzing.
        For example if a word starts at file position 498 and ends at file position 502, then because
        the threads each normally handles 500 char each, the word would be split between the threads
        and the program would see it as 2 different words, and the actual length would therefore not be
        recorded.
        The program instead looks at when the thread is supposed to stop, and checks if the last character
        is a seperator, if it isn't then it checks the next character until it finds a seperator, 
        and then tells the thread how many extra characters it needs to analyze with the offset value.
        in order to make sure each thread doesn't start reading at a point already analyzed by the
        previous thread, a seperate value offset_total is stored, which keeps track of how much is
        offset in total, which is used to track the start positions in the file of the threads*/
        char seperator_check[1];
        fseek(file, (thread_char_length * i) + offset_total, SEEK_SET);
        fread(seperator_check, 1, 1, file);
        int offset = 0;
        while (checkForSeperator(seperator_check[0]) == 0 && (thread_char_length * i) + offset != 0)
        {
            fseek(file, (thread_char_length * i) + offset_total, SEEK_SET);
            fread(seperator_check, 1, 1, file);
            offset++;
            offset_total++;
        }
        if (i > 0)
        {
            thread_data_array[i - 1].offset_thread_end = offset;
        }
        if (i == no_of_threads - 1)
        {
            thread_data_array[i].offset_thread_end = 0;
        }
        thread_data_array[i].offset_thread_total = offset_total;
    }
    /*creates the threads*/
    for (int i = 0; i < no_of_threads; i++)
    {
        pthread_create(&threads[i], NULL, checkForLongestWord, (void *)&thread_data_array[i]);
    }
    int longest_word;
    int word_count;
    /*joins the threads back into main and collects the threads conclusions of longest word in said thread,
    then compares each threads longest word, to find the files longest word*/
    for (int i = 0; i < no_of_threads; i++)
    {
        pthread_join(threads[i], NULL);
        if (longest_word < thread_data_array[i].result_word_size)
        {
            longest_word = thread_data_array[i].result_word_size;
        }
        word_count += thread_data_array[i].result_word_count;
    }
    printf("%s %d %s %d %s", "\nThe longest word is ", longest_word, " characters long. \nThere are a total of ", word_count, " words in the file. \n");
    fclose(file);
    fflush(stdout);
}
