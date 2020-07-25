/*
Set up parameters for each ensemble component by parsing Algo name and storing component specs as int in par_array. 
*/
void component_to_int(int *par_array, int par_count)
{
	char algo_name[16];
	strcpy(algo_name, Algo); //copy string because strtok would modify Algo identifier
	string par_code = "DFTLH"; // used to verify correct TLH naming convention
	if(strlen(par_code) != par_count)
		quit("Wrong par_code length in component_to_int function");
	string tmp;
	int i,j;
	for(i = 0; i < par_count; i++)
	{
		if(i == 0)
		{
			tmp = strtok(algo_name,"_");
			if(tmp[0] == 'M')
				par_array[i] = 1;
			else if(tmp[0] != 'R')
				quit("Component Name Incorrect - first char must be either M or R");
			i++; //increment within loop, since Direction and Factor are coded together in the first segment
		}
		else
		{
			tmp = strtok(0,"_");
			if(tmp[0] != par_code[i])
				quit("Component Name Incorrect - check T,L or H Position");
		}
		
		for(j = 1; j < strlen(tmp); j++) 
		{
			if(tmp[j] < '0' || tmp[j] > '9')
				quit("Invalid (non-numeric) Component Parameter Specification");
		}
		
		par_array[i] = atoi(tmp+1);
	}
}

/*
Function for using assets listed in inc_asset string. returns the number of assets to be used.
*/
int pick_assets_from_list(string *mod_asset_list, string inc_asset)
{
	int i = 0,j = 0;
	while(Assets[i])
	{
		if(!strstr(inc_asset, Assets[i]))
		{
			i++;
			continue;  // don't increment j
		}
		else 
		{
			mod_asset_list[j] = Assets[i];
			j++;
			i++;
		}
	}

	if(j < 2) quit("Fewer than 2 assets left after asset exclusion");
	
	return(j);
}


/*
Function for generating a custom asset list

AL_PATH - needs to be defined in 'rw_tools.c' as: #define AL_PATH "\\path\\to\\RWData\\FXBootcamp\\Zorro-Assets-Lists" 
				the define needs to be before the line: #include <modular.c>

base_list_name - the name of the asset list from which the entries for the wanted assets will be copied
new_list_name - the name of the asset list to which the selected entries from the base_list will be copied
wanted_assets - a string array containing the asset names that will be copied from base_list to new_list
*/
string filter_asset_list(string base_list_name, string new_list_name, string* wanted_assets) {

	string al_path = strf("%s\\%s%s", AL_PATH, base_list_name, ".csv");
	string new_al_path = strf("%s\\%s%s", AL_PATH, new_list_name, ".csv");
	
	int al_len = file_length(al_path);
	
	char* al_cont = malloc((al_len+1) * sizeof(char));
	
	if( al_cont == NULL ) {
		quit("!!! Error allocating 'al_cont' in 'filter_asset_list' !!!");
	}
	
	int read_bytes = file_read(al_path, al_cont, al_len);
	
	al_cont[read_bytes] = '\0';
	
	string line = 0;
	
	string* next_asset;

	while(1) {
		if ( line == 0 ){
			line = strtok(al_cont, "\n");
			file_write(new_al_path, strf("%s%s", line, "\n"), 0);
			continue;
		} else {
			line = strtok(0, "\n");
		}
		
		if ( line != 0 ) {
			next_asset = wanted_assets;
			while(*next_asset){
				if( strstr(line, *next_asset) ) {
					file_append(new_al_path, strf("%s%s", line, "\n"));
				}
				next_asset++;
			}
		} else {
			break;
		}
	}
	
	free(al_cont);
	al_cont = NULL;
	
	return new_al_path;
}	

/*
Function for parsing multiple strings of comma-separated asset names and combining all unique entries in a single array

unique_assets - array of strings where unique asset names will be copied into
						needs to be initialized to the maximum possible number of unique assets before calling this function
csv_assets_string - string of comma-separated asset names to be added to unique_assets
*/
void get_unique_assets(string *unique_assets, string csv_assets_string)
{
	string new_asset = 0;
	string asset_string;
	string* next_asset;
	
	asset_string = malloc( (strlen(csv_assets_string) + 1) * sizeof(char) );
	
	if ( asset_string == NULL ) {
		quit("!!! Error allocating 'asset_string' in 'get_unique_assets' !!!");
	}

	strcpy(asset_string, csv_assets_string);
			
	while(1) {
		if ( new_asset == 0 ) {
			new_asset = strtok(asset_string, ",");
		} else {
			new_asset = strtok(0, ",");
		}
		
		if ( new_asset != 0 ) {
			next_asset = unique_assets;
			
			while(*next_asset != NULL) {
				
				if ( strstr( *next_asset, new_asset) ) {
					new_asset = "NULL";
					break;
				}
				
				next_asset++;
			}
			
			if ( new_asset != "NULL" ) {
				*next_asset = malloc( (strlen(new_asset) + 1) * sizeof(char) );
				if ( *next_asset == NULL ) {
					quit("!!! Error allocating 'next_asset' in 'get_unique_assets' !!!");
				}
				strcpy(*next_asset, new_asset);
			}
		} else {
			break;
		}
	}

	free(asset_string);
	asset_string = NULL;
}

/*
Function for generating a custom asset list by copying the lines of wanted assets from a specified base list

base_list_name - the name of the asset list from which the entries for the wanted assets will be copied
new_list_name - the name of the asset list to which the selected entries from the base_list will be copied
						the asset list will be created if it doesn't exist, if it does it will be overwritten
csv_assets_strings - an array containing strings of comma-separated asset names to be copied from base_list
num_assets_strings - number of string elements in csv_assets_strings
*/
string create_asset_list(string base_list_name, string new_list_name, string* csv_assets_strings, int num_assets_strings) {
	string* unique_assets;
	
	int max_asset_num = 1; 
	int i;
	for(i = 0; i < num_assets_strings; i++) {
		max_asset_num += strcount(csv_assets_strings[i], ',') + 1;
	}
	
	unique_assets = malloc( max_asset_num * sizeof(char*) );
	
	if ( unique_assets == NULL ) {
		quit("!!! Error allocating 'unique_assets' in 'create_asset_list' !!!");
	}
	
	memset( unique_assets, 0, max_asset_num * sizeof(char*) );
	
	for(i = 0; i < num_assets_strings; i++) {
		get_unique_assets(unique_assets, csv_assets_strings[i]);
	}
	
	string new_al_list = filter_asset_list(base_list_name, new_list_name, unique_assets);
	
	i = 0;
	while(*(unique_assets+i)) {
		free(*(unique_assets+i));
		*(unique_assets+i) = NULL;
		i++;
	}
	
	free(unique_assets);
	unique_assets = NULL;
	
	return new_al_list;
}