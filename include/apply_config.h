void apply_config(char* conffile);
void print_env();
int env_i(char* name);
double env_d(char* name);
void set_int_param(struct pair ** config, char* name, int defval);
void set_dbl_param(struct pair ** config, char* name, double defval);
int test_env(char* block);
void milling_start(double val);
double norm_dia(double d);


