#include <curl/curl.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

void download_resource(char* resource);
_Bool arguments_introduced_fine(int argc, char *argv[]);
void ARGS_ERROR();
void parse_resource(char* resource);
char* build_full_file_name();
void set_file_name(char* resource, int last_slash_position, int last_period_position);
void set_file_extension(char* resource, int last_slash_position);

char FILE_NAME[500];
char FILE_EXT[10];
int main(int argc, char *argv[]) {

  char* resource;
  if(arguments_introduced_fine(argc, argv)){
      resource = argv[2];
      printf("\nResource asked = %s\n", resource);
      parse_resource(resource);
      download_resource(resource);
      printf("Resorce $GET was succesfull.\n");
  }
  else ARGS_ERROR();

  return 0;
}

void download_resource(char* resource){

  char* full_file_name = build_full_file_name();
	CURL *curl;
	CURLcode res;
	FILE *fp;

	curl = curl_easy_init();
	fp = fopen(full_file_name,"wb");

	curl_easy_setopt(curl, CURLOPT_URL, resource);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	fclose(fp);

}

_Bool arguments_introduced_fine(int argc, char *argv[]){
  return (argc == 3 && !strcmp(argv[1], "-u"));
}

void ARGS_ERROR(){
  printf("THE ARGUMENTS INTRODUCED ARE MISSING OR ARE WRONG\n");
}

void parse_resource(char* resource){
  int period_index = 0, slash_index = 0;
  int period_found = FALSE, slash_found = FALSE;
  int i = strlen(resource)-1;
  while (i >= 0) {
    if(slash_found && period_found) break;
    if(resource[i] == '.' && !period_found){
      period_index = i;
      period_found = TRUE;
    }
    if(resource[i] == '/' && !slash_found){
      slash_index = i;
      slash_found = TRUE;
    }
    i--;
  }
  set_file_name(resource, slash_index, period_index);
  set_file_extension(resource, period_index);
}

char* build_full_file_name(){
  return strcat(FILE_NAME, FILE_EXT);
}

void set_file_name(char* resource, int last_slash_position, int last_period_position){
  int num_chars = last_period_position - last_slash_position - 1;
  strncpy(FILE_NAME, resource+last_slash_position+1, num_chars);
  printf("Resource file name = %s\n", FILE_NAME);
}

void set_file_extension(char* resource, int last_period_position){
  strncpy(FILE_EXT, resource+last_period_position, strlen(resource) - last_period_position);
  printf("Resource file extension = %s\n\n", FILE_EXT);
}
