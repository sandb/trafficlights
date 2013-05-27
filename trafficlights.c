#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <stdio.h>
#include <linux/ppdev.h>
#include <asm/ioctl.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <json/json.h>
#include <json/json_tokener.h>
#include <argp.h>

#define LED_RED		0x03    /* 0011 */
#define LED_ORANGE	0x05   	/* 0101 */
#define LED_GREEN	0x06    /* 0110 */
#define LED_MASK	0x07	/* 0111 */
#define LED_BLINKING	0x08	/* 1000 */
#define LED_OFF		LED_MASK

#define BILLION 1000000000LL

/* The address of the (first) parallel port. */
const int addr = 0x378;

/*
 * traffic_set() - Sets the status of the traffic light.
 * 
 * @lights:	The status to set. Bitmask of LED_xxx constants.
 */
void traffic_set(int lights) 
{
	outw(lights & LED_MASK, addr);
}

/*
 * str_starts_with() - Helper function, returns true if str starts with prefix.
 *
 * @str: The string to check.
 * @prefix: The prefix to check for.
 */
int str_starts_with(const char *str, const char *prefix)
{
    size_t len_prefix = strlen(prefix);
    size_t len_str = strlen(str);
    return len_str < len_prefix ? 0 : strncmp(prefix, str, len_prefix) == 0;
}

/*
 * str_ends_with() - Helper function, returns true if str ends with suffix.
 *
 * @str: The string to check.
 * @suffix: The suffix to check for.
 */
int str_ends_with(const char *str, const char *suffix) 
{
    size_t len_suffix = strlen(suffix);
    size_t len_str = strlen(str);
    return len_str < len_suffix ? 0 : strncmp(suffix, &str[len_str - len_suffix], len_suffix) == 0;
}

/*
 * str_concatenate() - creates a new string containing concatenation of a and b, zero terminated.
 *
 * @a:	The first string.
 * @b:	The second string.
 */
char* str_concatenate(const char *a, const char *b) 
{
	size_t len_a = strlen(a);
	size_t len_b = strlen(b);
	char *result = malloc(len_a + len_b + 1);
	memcpy(result, a, len_a);
	memcpy(&result[len_a], b, len_b + 1);
	return result;
}

/*
 * nsleep() - Sleep for nsec nanoseconds.
 * 
 * @nsec:	Number of nanoseconds to sleep.
 */
void nsleep(long long nsec) 
{
	long nan = nsec % BILLION;
	time_t sec = nsec / BILLION;
	struct timespec rqtp = { .tv_nsec = nan, .tv_sec = sec };
	int res = nanosleep(&rqtp, NULL);
	if (res != 0) 
	{
		fprintf(stderr, "%d is return, EINTR %d, EINVAL\n", res, errno == EINTR, errno == EINVAL);
	}
}

/*
 * msleep() - Sleep for msec miliseconds.
 *
 * @msec:	Number of miliseconds to sleep.
 */
void msleep(long msec) 
{
	nsleep(msec * 1000000LL);
}

typedef struct 
{
	char* json;
	size_t size;
} JENKINS_JSON;

/*
 * jenkins_curl_write_function() - Curl write function that receives jenkins data.
 * 
 * @ptr:	The data as chars. Not zero terminated.
 * @size:	Size of a character in bytes. 
 * @nmemb:	Number of characters in ptr. Multiply by size to get memsize in bytes.
 * @userdata:	Argument set with the CURLOPT_WRITEDATA option.
 * 
 * Returns the number of bytes actually taken care of. If that amount differs 
 * from the amount passed, it'll signal an error to the library. This will 
 * abort the transfer and return CURLE_WRITE_ERROR. 
 */
size_t jenkins_curl_write_function(char *ptr, size_t size, size_t nmemb, void *userdata) 
{
	/* size in bytes */
	size_t ptr_size = size * nmemb;

	/* JENKINS_JSON struct access to untyped userdata pointer */
	JENKINS_JSON *j = (JENKINS_JSON*) userdata;

	/* Enlarge buffer size to new size + 1 for zero termination */
	j->json = realloc(j->json, j->size + ptr_size + 1);

	/* Add new chunk to existing buffer */
	memcpy(&(j->json[j->size]), ptr, ptr_size);

	/* Update size of JENKINS_JSON struct (one less than actually allocated, so not including zero char) */
	j->size += ptr_size;

	/* Ensure zero-termination, just in case */
	j->json[j->size] = '\0';

	/* return processed bytes */
	return ptr_size;
}

/*
 * jenkins_get_status() - Get json describing jenkins status.
 *
 * @jenkins_url:	The url of the jenkins server in the form of 
 * 			http://<domainname>[:<port>]. /api/json will be added.
 *
 * Gets the json data containing the status of the server and all plans on the 
 * server as a zero terminated string. If not retrievable, returns NULL.
 */
char* jenkins_get_status(char* jenkins_url) 
{
	CURL *curl;
	CURLcode res = 0;
	JENKINS_JSON json = { NULL, 0 };

	char *url = str_concatenate(jenkins_url, "/api/json");

	curl = curl_easy_init();
	if(!curl) return NULL;

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, jenkins_curl_write_function);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);
	res = curl_easy_perform(curl);

	curl_easy_cleanup(curl);
	free(url);

	return json.json;
}

/*
 * jenkins_color_atoi() - converts a string status to its traffic light counterpart.
 *
 * @color:	A string with a jenkins color status.
 *
 * Converts a string denoting a status of a jenkins job to its numeric 
 * led status counterpart. 
 */
int jenkins_color_to_led(const char* color) 
{
	int status = 0;
	if (str_starts_with(color, "blue")) status = LED_GREEN;
	if (str_starts_with(color, "yellow")) status = LED_ORANGE;
	if (str_starts_with(color, "red")) status = LED_RED;
	if (str_starts_with(color, "disabled")) status = LED_OFF;
	if (str_starts_with(color, "grey")) status = LED_GREEN | LED_ORANGE | LED_RED | LED_BLINKING;
	if (str_ends_with(color, "anime")) status |= LED_BLINKING;
	return status;
}

/*
 * jenkins_get_job_status() - Return the traffic light status of a specific plan.
 * 
 * @jenkins_json:	The status of the the jenkins server as a json string.
 * @jenkins_job:	The name of the plan for which to return the status.
 */
int jenkins_get_job_status(const char* jenkins_json, const char* jenkins_job) 
{
	json_object *jenkins_status = json_tokener_parse(jenkins_json);

	if (jenkins_status == NULL)
	{
		fprintf(stderr, "Error: couldn't parse returned message from server as valid json\n");
		return LED_BLINKING;
	}

	json_object *jobs = json_object_object_get(jenkins_status, "jobs");
	size_t num_jobs = json_object_array_length(jobs);
	int job_status = 0;
	int i;
	for (i = 0; i < num_jobs; i++) 
	{
		json_object *job = json_object_array_get_idx(jobs, i);
		const char *name = json_object_get_string(json_object_object_get(job, "name"));
		if (strcasecmp(name, jenkins_job) != 0) continue;
		const char *color = json_object_get_string(json_object_object_get(job, "color"));
		job_status = jenkins_color_to_led(color);
		break;
	}	
	/* dereference/free the json object */
	json_object_put(jenkins_status);

	if (job_status == 0) 
	{
		fprintf(stderr, "Error: Unknown job name: %s\n", jenkins_job);
		return LED_BLINKING;
	}
	return job_status;
}

/** argparse global variables */
const char *argp_program_version = "trafficlights 1.0";
const char *argp_program_bug_address = "<pieter.iserbyt@gmail.com>";
static char doc[] = "trafficlights -- monitors a jenkins job and outputs the status on a trafficlight connected to the parallel port";

/** argparse defention of commandline options */
static struct argp_option options[] = 
{
	{"refreshrate",	'r',	"seconds",	0,	"After how many seconds do we update the status of a job" },
	{"job",		'j',	"jobname",	0,	"The name of the job to monitor" },
	{"server",	's',	"server url",	0,	"The url of the jenkins server" },
	{ 0 }
};

/** struct that will receive commandline specified settings, passed to callback function */
struct arguments 
{
	int rate;
	char *job;
	char *server;
};

/** 
 * parse_opt() - argparse callback function.
 * 
 * @key:	The commandline key of the commandline option being processed.
 * @arg:	The specified argument value.
 * @argp_state:	Pointer to argp_option struct that will be updated with specified values.
 * 
 * Callback function that handles command line options.
 */
static error_t parse_opt (int key, char *arg, struct argp_state *state) 
{
	struct arguments *args = state->input;
	switch (key)
	{
		case 'r':
			args->rate = atoi(arg);
			break;
		case 'j':
			args->job = arg;
			break;
		case 's':
			args->server = arg;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/** argparse struct holding options, callback and documentation references. */
static struct argp argp = { options, parse_opt, NULL, doc };

int main(const int argc, char** argv) 
{

	struct arguments arguments = { 30, NULL, NULL};
	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	if (arguments.job == NULL) 
	{
		fprintf(stderr, "No job name specified\n");
		return -1;
	}

	if (arguments.server == NULL) 
	{
		fprintf(stderr, "No server url specified\n");
		return -1;
	}

	printf("Trafficlights!\n");

	int parportfd = open("/dev/parport0", O_RDWR);
	ioctl(parportfd,PPCLAIM);
	ioctl(parportfd,PPRELEASE);
	ioperm(addr,5,1);

	printf("Trafficlight initialized.\n");

	char c;
	clock_t start = clock();
	int term = fcntl(0, F_GETFL, 0);
	fcntl (0, F_SETFL, (term | O_NDELAY));

	int counter = 0;
	int status;

	printf("Trafficlight loop started.\n");
	printf("Updating status every %d seconds.\n", arguments.rate);
	printf("Jobname is %s\n", arguments.job);
	printf("Server is %s\n", arguments.server);
	while (1) 
	{
		if (counter > arguments.rate) counter = 0;
		
		if (counter == 0) 
		{
			char *jenkins_status = jenkins_get_status(arguments.server);
			if (jenkins_status == NULL) 
			{
				if (status != LED_BLINKING)
				{
					fprintf(stderr, "Could not retrieve jenkins status from [%s], retrying.\n", arguments.server);
				}
				status = LED_BLINKING;
				break;
			}
			status = jenkins_get_job_status(jenkins_status, arguments.job);
			free(jenkins_status);
		}
		
		if (status & LED_BLINKING) 
		{
			traffic_set(status);
			msleep(500);
			traffic_set(LED_OFF);
			msleep(500);
		} 
		else 
		{
			traffic_set(status);
			msleep(1000);
		}
		counter++;
	}
	fcntl(0, F_SETFL, term);
	close(parportfd);
}
