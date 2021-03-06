#include "constants.h"

void index(c* conn, r* request_info, void* user_data)
{
    request_info = request_info;
    user_data = user_data;
    head(conn, 200);
    say(conn, "X-ray Optical Constants Daemon " GITVERSION);
}

void all(c* conn, r* request_info, void* user_data)
{
    user_data = user_data;
    char* material = request_info->uri + 1;
    std::string answer;
    std::string matfile = path + "/" + material + ".nk";

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
    std::string answer;

    char* material = request_info->uri + 1;
    char* energy = strstr(material, "/") + 1;

    double wavelength = 12.398 / atof(energy); /* assume keV */

    if ( (!strstr(energy, "keV")) && (!strstr(energy, "kev")) )
	wavelength *= 1000;

    strstr(material, "/")[0] = '\0';
    std::string matfile = path + "/" + material + ".nk";

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
    std::string answer;

    char* material = request_info->uri + 1;
    char* wavelength = strstr(material, "/") + 1;

    strstr(material, "/")[0] = '\0';
    std::string matfile = path + "/" + material + ".nk";

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

int main(int argc, char* const* argv)
{

    int flag_daemon = 0;
    const char* opath = "./data";
    const char* oport = "42001";
    int c;
    char hostname[65];
    gethostname(hostname, sizeof hostname);

    opterr = 0;

    while ((c = getopt(argc, argv, "df:hp:")) != -1)
	switch (c)
	{
	    case 'h':
		std::cout << "X-ray Optical Constants Daemon (xocd) serves the index of refraction" << std::endl <<
		    "for various elements and compounds via HTTP to your applications." << std::endl <<
		    std::endl <<
		    "The data is read from the *.nk-files of the IMD suite, located" << std::endl <<
		    "in a folder './data', or given by the option -f <folder>." << std::endl <<
		    "The IMD software can be obtained from David Windt [1] or the ESRF [2]." << std::endl <<
		    "Example files for silicon (Si), tungsten (W) and boron carbide (B4C)," << std::endl <<
		    "as well as vacuum (vac) are provided." << std::endl <<
		    "The http server part is taken from the mongoose project [3]." << std::endl << std::endl <<
		    std::endl <<
		    "xocd can is written by Markus Osterhoff and can be optain from [4,5]." << std::endl <<
		    std::endl <<
		    std::endl <<
		    "\tusage: " << argv[0] << " [-d] [-f <folder>] [-p port] [-h]" << std::endl <<
		    std::endl <<
		    "\t-d: daemonize, fork into background; default: don't" << std::endl <<
		    "\t-f: use *.nk-files in <folder>; default: " << opath << std::endl <<
		    "\t-p: listen on <port> for HTTP connections; default: " << oport << std::endl <<
		    "\t-h: help; this is me." << std::endl << 
		    std::endl <<
		    "How to get the data:" << std::endl <<
		    "browse to http://" << hostname << ":" << oport << "/ -- with the following URIs:" << std::endl <<
		    "\t/Si        -- ask for whole Si.nk-file" << std::endl <<
		    "\t/Si/17keV  -- ask for index of refraction at 17 keV. " << std::endl <<
		    "\t/Si/8048eV -- ask for index of refraction at 8048 eV." << std::endl <<
		    "\t/Si/0.1nm  -- ask for index of refraction at 0.1 nm. " << std::endl <<
		    std::endl <<
		    "The output for specific energies / wavelengths will be of the form" << std::endl <<
		    "\t1 -(delta) +(beta)i" << std::endl <<
		    std::endl <<
		    "[1] http://cletus.phys.columbia.edu/~windt/idl/" << std::endl <<
		    "[2] http://ftp.esrf.eu/pub/scisoft/xop2.1/Unix/Extensions/" << std::endl <<
		    "[3] http://code.google.com/p/mongoose/" << std::endl <<
		    "[4] http://www.roentgen.physik.uni-goettingen.de/~mosterh" << std::endl <<
		    "[5] http://github.com/osti/xocd" << std::endl <<
		    std::endl ;
		exit(0);
	    case 'd':
		flag_daemon = 1;
		break;
	    case 'f':
		opath = optarg;
		break;
	    case 'p':
		oport = optarg;
		break;
	    case '?':
		std::cerr << "error parsing " << optopt << "." << std::endl;
		return -1;
	    default:
		return -1;
	}

    int exit_flag = 0;

    path = opath;
    port = oport;

    if (flag_daemon)
    {
	pid_t i = fork();
	if (i<0)
	    exit(1); /* fork error */
	if (i>0)
	{
	    std::cout << "X-ray Optical Constants Daemon running in background..." << std::endl;
	    exit(0); /* parent exits */
	}

    }

    ctx = mg_start();
    mg_set_option(ctx, "ports", port.c_str());
    mg_set_uri_callback(ctx, "/"            ,  &index        , NULL);
    mg_set_uri_callback(ctx, "/*/*eV"       ,  &energy       , NULL);
    mg_set_uri_callback(ctx, "/*/*ev"       ,  &energy       , NULL);
    mg_set_uri_callback(ctx, "/*/*"         ,  &wavel        , NULL);
    mg_set_uri_callback(ctx, "/*"           ,  &all          , NULL);

    if (!flag_daemon)
	std::cout << "X-ray Optical Constants Daemon running..." << std::endl;

    while (exit_flag == 0)
	sleep(1);

    mg_stop(ctx);

    return 0;
}

