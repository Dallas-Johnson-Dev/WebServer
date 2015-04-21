#include <iostream> // cout, cerr, etc
#include <fstream>
#include <stdio.h>		// perror
#include <string.h>		// bcopy
#include <netinet/in.h>		// struct sockaddr_in
#include <unistd.h>		// read, write, etc
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sstream>
#include <string>
#include <dirent.h>

using namespace std;

const int BUFSIZE=1024; //Limits the buffer size to 1024 bytes.

void Error(const char *msg) //Allows for error checking.
{
	perror(msg);
	exit(1);
}

int MakeServerSocket(char *port) {
	const int MAXNAMELEN = 255; //Limits the name of the host to 255 characters.
	const int BACKLOG = 3; //Limits the amount of clients that can wait for the server to accept the connection.
	char localhostname[MAXNAMELEN]; //Stores the hostname entered by the user as an array of chars.
	int s; //Determines whether an error has occured creating the socket.  If it has, -1 will be returned.
	struct sockaddr_in sa; //Retreives the socket address.
	struct hostent *hp; //Determines whether the client is connected.
	struct servent *sp; //Determines whether the server is connected.
	int portnum; //Determines whether the portnumber is valid.
	int ret; //Determines if the socket address is valid.

	gethostname(localhostname,MAXNAMELEN);
	hp = gethostbyname(localhostname);
	if (hp == NULL) Error("GetHostByName");

	sscanf(port, "%d", &portnum);
	if (portnum ==  0) {
		sp=getservbyname(port, "tcp");
		portnum = ntohs(sp->s_port);
	}
	sa.sin_port = htons(portnum);

	bcopy((char *)hp->h_addr, (char *)&sa.sin_addr, hp->h_length);
	sa.sin_family = hp->h_addrtype;

	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s < 0) Error("Socket");

	ret = bind(s, (struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0) Error("Bind");

	listen(s, BACKLOG);
	cout << "Waiting for connection on port " << port << " hostname " << localhostname << endl;
	return s;
}

int main(int argc, char *argv[]) {


	int s; //Contains the socket.
	int len; //Determines the number of bytes to be read.
	char buf[BUFSIZE]; //Contains the characters read in to the server.
	int ret; //Determines if a Write Error has occured.
	int fd; //Describes the session created when a request is accepted.
	string type; //Determines the type of file the user is requesting.
	int file;
	int writefile = open("logfile.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	s = MakeServerSocket(argv[1]);
	if (s < 0) Error("MakeServerSocket");
	while (1) {
		struct sockaddr_in sa;
		int sa_len = sizeof(sa);
		fd = accept(s, (struct sockaddr *)&sa, (unsigned int *)&sa_len);
		if (fd < 0) Error("Accept");
		int pid = fork();
		if (pid < 0) Error ("forking gone wrong");
		if (pid == 0){//You are the child.
			string get, filename, version;
			len = read(fd, buf, BUFSIZE);
			if (len < 0) Error("Bad Read");
			istringstream is(buf);
			is >> get;
			is >> filename;
			is >> version;
			cout << get << " " << filename << " " << version << "\n";
			filename = "." + filename;
			string thing = "HTTP/1.0 200 OK \r\n";
			len = thing.length();
			len = write(fd, thing.c_str(), len);
			thing = "HTTP/1.0 200 OK \r\n";
			len = thing.length();
			len = write(writefile,buf,len);
			if (len < 0)Error("Bad Write");
			if (filename == "./~ashimun"){
				filename = "/home/ashimun/current/cs228";
				string test = "\r\n";
				len = write(writefile,buf,len);
				if (len < 0) Error("Bad File");
				DIR * directory = opendir(filename.c_str());
				if(!directory) Error("No Directory");
				struct dirent * file;
				while ( file = readdir(directory)){
					string filename = string(file->d_name) + "\r\n";
					test += filename;
				}
				len = write(fd,test.c_str(),test.length());
				closedir(directory);
			}
			else{
				file = open(filename.c_str(), O_RDONLY);
				if (file < 0){
					filename = "./404.html";
					file = open(filename.c_str(), O_RDONLY);
				}
				len = filename.length();
				for (len = len -1; len >-1; len--){
					if (filename[len] == '.'){
						type = filename.substr(len+1);
						break;
					}
				}
				if (type == "txt") type = "text/plain";
				else if (type == "html") type = "text/html";
				else if (type == "gif") type = "image/gif";
				else if (type == "png") type = "image/png";
				else if (type == "jpeg" || type == "jpg") type = "image/jpeg";
				thing = "Content-type: " + type + "\r\n\r\n";
				len = thing.length();
				len = write (fd, thing.c_str(),len);
				while (len > 0){
					len = read(file,buf,BUFSIZE);
					if (len < 0) Error("Bad File Read");
					buf[len] = 0;
					cout << buf << endl;
					len = write(fd,buf,len);
					if (len < 0) Error("Bad File Write");
				}
			}
			exit(1);
		}
		else if (pid > 0){
			int status = 0, w = 999;
			do{
				w = waitpid(-1,&status,WNOHANG);
			}while(w > 0);
		close(fd);
		}
	}
}
