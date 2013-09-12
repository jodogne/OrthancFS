#ifndef __ORTHANCFS_H__
#define __ORTHANCFS_H__
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <iostream>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <map>
#include "OrthancCppClient.h"

const char * LIBORTHANCADRESS = "lib/libOrthancClient.so";
/**
The paths given in Orthanc can only be of 5 types:
	ROOT, when the string is /
	PATIENT, when the string is /patientname
	STUDY, when the string is /patientname/study
	SERIES, when the string is /patientname/study/series/
	INSTANCE when the string is /patientname/study/series/instance
	or ERROR when it is not one of these 5
	
It is easy to check thanks to the /, but as some repositories are added like
/.Trash/1000/, checking the / is not the ultimate way to check the validity
*/
#define OFS_PATIENT  0
#define OFS_STUDY    1
#define OFS_SERIES   2
#define OFS_INSTANCE 3
#define OFS_ROOT     4
#define OFS_ERROR    -1

using namespace std;

/**
	There are four maps, one for each level of the infrastructure.
	Each time a directory is read, the corresponding patient/study/... is read
	in orthanc, its path is added in one of the four maps with the corresponding
	number it has been selected with GetPatient(i).
	This way, it is extremely easy to recover the study of a serie and so on.
*/
class OrthancFS{
	private:
		OrthancClient::OrthancConnection * orthanc;
		map<string,int> map_patient;
		map<string,int> map_study;
		map<string,int> map_series;
		map<string,int> map_instance;
		int get_o_pid(const char * path);
		int get_o_stid(const char * path);
		int get_o_seid(const char * path);
		int get_o_iid(const char * path);
	public:
		OrthancFS(const char * address);
		~OrthancFS();
		int getAttr(const char *path, struct stat *stbuf);
		int readDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
		int open(const char *path, struct fuse_file_info *fi);
		int read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi);
};

struct ofs_state {
    OrthancFS * ofs;
};
#define OFS_DATA ((struct ofs_state *) fuse_get_context()->private_data)


#endif
