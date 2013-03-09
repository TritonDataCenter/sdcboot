#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <dos.h>
#include <assert.h>
#include <malloc.h>

#define BUFFER_SIZE 128

#define cases_file_name		"cases.txt"
#define commands_file_name	"commands.txt"

#define config_sys_file_name	"c:\\config.sys"
#define autoexec_bat_file_name	"c:\\autoexec.bat"

#define output_dir		"output\\"
#define output_extension	".txt"

typedef enum {
    GET_NEXT_CASE_RESULT_OK,
    GET_NEXT_CASE_RESULT_NOTEXIST,
    GET_NEXT_CASE_RESULT_NONE,
    GET_NEXT_CASE_RESULT_END
} get_next_case_result_t;

typedef enum {
    GET_NEXT_CASE_STATE_FIND_CUR,
    GET_NEXT_CASE_STATE_GET_NEXT,
    GET_NEXT_CASE_STATE_DONE
} get_next_case_state_t;

typedef int boolean;
#define FALSE (0)
#define TRUE (!FALSE)

get_next_case_result_t get_next_case(char *cur_case, char **next_case,
				     unsigned int *next_case_num,
				     unsigned int *case_count)
{
    FILE *fp_cases;
    char buffer[BUFFER_SIZE];
    get_next_case_state_t state;
    unsigned int eol_pos;

    fp_cases = fopen(cases_file_name, "rt");
    if (fp_cases == NULL) {
	fprintf(stderr, "Could not open %s, are you in the right "
		"directory?\n", cases_file_name);
	exit(0);
    }

    /*
     * If we don't have a current case, we want to return the first one,
     * otherwise we have to find the current case and then return the next
     * one.
     */
    if (cur_case == NULL) {
	state = GET_NEXT_CASE_STATE_GET_NEXT;
    } else {
	state = GET_NEXT_CASE_STATE_FIND_CUR;
    }

    *case_count = 0;
    while (fgets(buffer, BUFFER_SIZE, fp_cases) != NULL) {
	/*
	 * Skip blank lines and trim end-of-line character.
	 */
	if (strlen(buffer) < 1)	{
	    continue;
	}
	eol_pos = strlen(buffer) - 1;
	if (buffer[eol_pos] == '\n') {
	    if (eol_pos == 0) {
		continue;
	    }
	    buffer[eol_pos] = '\0';
	}

	switch (state) {
	case GET_NEXT_CASE_STATE_FIND_CUR:
	    assert(cur_case != NULL);
	    if (strcmp(cur_case, buffer) == 0) {
		state = GET_NEXT_CASE_STATE_GET_NEXT;
	    }
	    break;
	case GET_NEXT_CASE_STATE_GET_NEXT:
	    *next_case = strdup(buffer);
	    *next_case_num = *case_count;
	    state = GET_NEXT_CASE_STATE_DONE;
	    break;
	case GET_NEXT_CASE_STATE_DONE:
	    break; /* do nothing, we just want to count */
	}
	(*case_count)++;
    }
    fclose(fp_cases);

    if (*case_count == 0) {
	return GET_NEXT_CASE_RESULT_NONE;
    }
    switch (state) {
    case GET_NEXT_CASE_STATE_FIND_CUR:
	assert(cur_case != NULL);
	return GET_NEXT_CASE_RESULT_NOTEXIST;
    case GET_NEXT_CASE_STATE_GET_NEXT:
	/*
	 * This assertion should be valid as if cur_case is NULL, we wanted
	 * to get the first case, and we could only fail to do that if there
	 * were no cases, and we already checked for *case_count == 0 above.
	 */
	assert(cur_case != NULL);
	*next_case = NULL;
	return GET_NEXT_CASE_RESULT_END;
    case GET_NEXT_CASE_STATE_DONE:
	break; /* handled below */
    }

    return GET_NEXT_CASE_RESULT_OK;
}

#define COPY_FILE_BUFFER_SIZE 1024

void copy_file(char *src, char *dst)
{
    char *buffer;
    unsigned int bytes;
    FILE *fp_src, *fp_dst;

    buffer = malloc(COPY_FILE_BUFFER_SIZE);
    if (buffer == NULL)	{
	fprintf(stderr, "Memory allocation failure\n");
	exit(0);
    }
    fp_src = fopen(src, "rb");
    if (fp_src == NULL)	{
	fprintf(stderr, "Failed to open %s for input\n", src);
	exit(0);
    }
    fp_dst = fopen(dst, "wb");
    if (fp_dst == NULL)	{
	fprintf(stderr, "Failed to open %s for output\n", dst);
	fclose(fp_src);
	exit(0);
    }

    bytes = COPY_FILE_BUFFER_SIZE;
    while (bytes == COPY_FILE_BUFFER_SIZE) {
	bytes = fread(buffer, 1, bytes, fp_src);
	if (fwrite(buffer, 1, bytes, fp_dst) != bytes) {
	    fprintf(stderr, "Error writing to %s\n", dst);
	    fclose(fp_src);
	    fclose(fp_dst);
	    exit(0);
	}
    }

    fclose(fp_src);
    fclose(fp_dst);
}

void copy_file_with_msg(const char *msg, char *src, char *dst)
{
    printf(msg, src, dst);
    copy_file(src, dst);
}

void backup_startup_files(void)
{
    static const char *msg = "Backing up %s to %s...\n";

    copy_file_with_msg(msg, config_sys_file_name, "config.org");
    copy_file_with_msg(msg, autoexec_bat_file_name, "autoexec.org");
}

void restore_startup_files(void)
{
    static const char *msg = "Restoring %s to %s...\n";

    copy_file_with_msg(msg, "config.org", config_sys_file_name);
    copy_file_with_msg(msg, "autoexec.org", autoexec_bat_file_name);
}

/*
 * Copy <case>.sys to config.sys
 */
void write_config_sys(char *next_case)
{
    char *src;
    static const char *ext = ".sys";

    src = malloc(strlen(next_case) + strlen(ext) + 1);
    if (src == NULL) {
	fprintf(stderr, "Memory allocation failure\n");
	exit(0);
    }
    strcpy(src, next_case);
    strcat(src, ext);
    copy_file_with_msg("Copying %s to %s...\n", src, config_sys_file_name);
    free(src);
}

#define DIR_BUFFER_SIZE 256

/*
 * Generate autoexec.bat from <case>.bat and commands.txt
 */
void write_autoexec_bat(char *next_case)
{
    char *src;
    static const char *ext = ".bat";
    FILE *fp_src, *fp_dst;
    char buffer[BUFFER_SIZE];
    char *output_file_name;
    char dir[DIR_BUFFER_SIZE];
    unsigned int eol_pos;

    if (_getcwd(dir, DIR_BUFFER_SIZE) == NULL) {
	fprintf(stderr, "Failed to get current directory\n");
	exit(0);
    }

    src = malloc(strlen(next_case) + strlen(ext) + 1);
    if (src == NULL) {
	fprintf(stderr, "Memory allocation failure\n");
	exit(0);
    }
    strcpy(src, next_case);
    strcat(src, ext);
    copy_file_with_msg("Copying %s to %s...\n", src, autoexec_bat_file_name);
    free(src);

    printf("Appending commands to %s...\n", autoexec_bat_file_name);
    fp_src = fopen(commands_file_name, "rt");
    if (fp_src == NULL)	{
	fprintf(stderr, "Failed to open %s for input\n", commands_file_name);
	exit(0);
    }
    fp_dst = fopen(autoexec_bat_file_name, "at");
    if (fp_dst == NULL)	{
	fprintf(stderr, "Failed to open %s for append\n", commands_file_name);
	fclose(fp_src);
	exit(0);
    }

    output_file_name = malloc(strlen(output_dir) + strlen(next_case)
			      + strlen(output_extension) + 1);
    strcpy(output_file_name, output_dir);
    strcat(output_file_name, next_case);
    strcat(output_file_name, output_extension);

    fprintf(fp_dst, "\n");
    fprintf(fp_dst, "cd %s\n", dir);
    fprintf(fp_dst, "echo ========== Case: %s > %s\n", next_case,
	    output_file_name);

    while (fgets(buffer, BUFFER_SIZE, fp_src) != NULL) {
	/*
	 * Skip blank lines and trim end-of-line character.
	 */
	if (strlen(buffer) < 1)	{
	    continue;
	}
	eol_pos = strlen(buffer) - 1;
	if (buffer[eol_pos] == '\n') {
	    if (eol_pos == 0) {
		continue;
	    }
	    buffer[eol_pos] = '\0';
	}

	fprintf(fp_dst, "echo ========== Command: %s >> %s\n", buffer,
		output_file_name);
	fprintf(fp_dst, "..\\%s >> %s\n", buffer, output_file_name);
	fprintf(fp_dst, "echo ========== End of command output >> %s\n",
		output_file_name);
    }
    fprintf(fp_dst, "memtest2 %s\n", next_case);
    fclose(fp_src);
    fclose(fp_dst);
}


int main(int argc, char *argv[])
{
    char *cur_case, *next_case;
    unsigned int next_case_num, case_count;

    if (argc > 2) {
	fprintf(stderr,
		"Error: expected no more than 1 command-line argument\n");
	exit(0);
    }
    if (argc == 2) {
	cur_case = argv[1];
    } else {
	cur_case = NULL;
	backup_startup_files();
    }

    switch (get_next_case(cur_case, &next_case, &next_case_num, &case_count)) {
    case GET_NEXT_CASE_RESULT_OK:
	break; /* continue processing below */
    case GET_NEXT_CASE_RESULT_NOTEXIST:
	assert(cur_case != NULL);
	fprintf(stderr, "Error: case %s specified on the command line "
		"does not seem to exist in %s\n", cur_case, cases_file_name);
	exit(0);
    case GET_NEXT_CASE_RESULT_NONE:
	fprintf(stderr, "Error: no cases appear to be defined in %s\n",
		cases_file_name);
	exit(0);
    case GET_NEXT_CASE_RESULT_END:
	break; /* continue processing below */
    }

    if (next_case == NULL) {
	printf("The last case has been completed\n");
	restore_startup_files();
    } else {
	printf("Next case is %s (case %u of %u)\n", next_case,
	       next_case_num + 1, case_count);
	write_config_sys(next_case);
	write_autoexec_bat(next_case);
	free(next_case);
    }

    return 0;
}
