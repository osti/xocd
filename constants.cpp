#include "constants.h"

void index(c* conn, r* request_info, void* user_data)
{
    request_info = request_info;
    user_data = user_data;
    head(conn, 200);
    say(conn, "running.\r\n");
}

void all(c* conn, r* request_info, void* user_data)
{
    user_data = user_data;
    char* material = request_info->uri + 1;
    string answer;
    string matfile = path + "/" + material + ".nk";

    if ( access( matfile.c_str(), R_OK ) == 0 )
    {
	FILE* stream = fopen(matfile.c_str(), "r");
	char buf[128];

	if (!stream)
	{
	    head(conn, 500);
	    answer = "could not open file.\r\n";
	    return;
	}
	head(conn, 200);
	while (!feof(stream))
	{
	    if (fgets(buf, sizeof(buf), stream) != 0)
		say(conn, buf);
	}
	fclose(stream);
    }
    else
    {
	head(conn, 404);
	answer = "file not found.\r\n";
    }
    say(conn, answer.c_str());
}

void energy(c* conn, r* request_info, void* user_data)
{
    user_data = user_data;
    request_info = request_info;
    string answer;

    char* material = request_info->uri + 1;
    char* energy = strstr(material, "/") + 1;

    double wavelength = 12.398 / atof(energy); /* assume keV */

    if ( (!strstr(energy, "keV")) && (!strstr(energy, "kev")) )
	wavelength *= 1000;

    strstr(material, "/")[0] = '\0';
    string matfile = path + "/" + material + ".nk";

    char* result = interpolate(matfile.c_str(), wavelength);

    if (result)
    {
	head(conn, 200);
	say(conn, result);
	free(result);
    }
    else
    {
	head(conn, 404);
	say(conn, "file not found");
    }
}

void wavel(c* conn, r* request_info, void* user_data)
{
    user_data = user_data;
    request_info = request_info;
    string answer;

    char* material = request_info->uri + 1;
    char* wavelength = strstr(material, "/") + 1;

    strstr(material, "/")[0] = '\0';
    string matfile = path + "/" + material + ".nk";

    char* result = interpolate(matfile.c_str(), atof(wavelength)*10);

    if (result)
    {
	head(conn, 200);
	say(conn, result);
	free(result);
    }
    else
    {
	head(conn, 404);
	say(conn, "file not found");
    }
}

void say(struct mg_connection* conn, const char* s)
{
    mg_printf(conn, s);
}

void head(struct mg_connection* conn, int z)
{
    mg_printf(conn, "%s %d %s\r\n%s",
	    "HTTP/1.1", z, "OK",
	    "Content-Type: text/plain\r\n\r\n");
}

char* interpolate(const char* matfile, double lambda)
{
    if ( access( matfile, R_OK ) == 0 )
    {
	FILE* stream = fopen(matfile, "r");
	char buf[128];

	if (!stream)
	    return 0;

	float Lambda1, N1, K1;
	float Lambda2, N2, K2;
	Lambda1 = 0;
	N1      = 1;
	K1      = 0;
	while (!feof(stream))
	{
	    float Lambda,  N,  K;
	    int ret;

	    ret = fscanf(stream, "%f %f %f\n", &Lambda, &N, &K);

	    if (ret == 3)
	    {
		Lambda2 = Lambda1;
		     N2 =      N1;
		     K2 =      K1;
		Lambda1 = Lambda ;
		     N1 =      N ;
		     K1 =      K ;

		if (Lambda1 > lambda && Lambda2 <= lambda)
		{
		    char* buf;
		    double delta, beta, y, m, h;
		    h = lambda - Lambda2;

		    y     =  (1-N2);
		    m     = ((1-N1)-(1-N2)) / (Lambda1-Lambda2);
		    delta = y + h*m;
		    y     =  ( K2 );
		    m     = (( K1 )-( K2 )) / (Lambda1-Lambda2);
		    beta  = y + h*m;

		    buf = (char*) malloc(sizeof(char) * 64);
		    snprintf(buf, 64, "1 %+7.3e %+7.3e i\r\n", -delta, beta);

		    fclose(stream);
		    return buf;
		}
	    }
	    else
		if (fgets(buf, sizeof(buf), stream) == 0)
		    return 0;
	}
	fclose(stream);
    }

    return 0;
}

int main(int argc, const char *argv[])
{
    int exit_flag = 0;

    if (argc == 3)
    {
	path = argv[1];
	port = argv[2];
    }
    else
    {
	path = "./data";
	port = "42001";
    }
    ctx = mg_start();
    mg_set_option(ctx, "ports", port.c_str());
    mg_set_uri_callback(ctx, "/"            ,  &index        , NULL);
    //mg_set_uri_callback(ctx, "/*/*eV/beta"  ,  &energy_beta  , NULL);
    //mg_set_uri_callback(ctx, "/*/*ev/beta"  ,  &energy_beta  , NULL);
    //mg_set_uri_callback(ctx, "/*/*/beta"    ,  &wavel_beta   , NULL);
    //mg_set_uri_callback(ctx, "/*/*eV/delta" ,  &energy_delta , NULL);
    //mg_set_uri_callback(ctx, "/*/*ev/delta" ,  &energy_delta , NULL);
    //mg_set_uri_callback(ctx, "/*/*/delta"   ,  &wavel_delta  , NULL);
    mg_set_uri_callback(ctx, "/*/*eV"       ,  &energy       , NULL);
    mg_set_uri_callback(ctx, "/*/*ev"       ,  &energy       , NULL);
    mg_set_uri_callback(ctx, "/*/*"         ,  &wavel        , NULL);
    mg_set_uri_callback(ctx, "/*"           ,  &all          , NULL);

    cout << "optical constants server running..." << endl;

    while (exit_flag == 0)
	sleep(1);

    mg_stop(ctx);

    return 0;
}

