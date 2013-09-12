#ifndef __ORTHANC_HASH_H__
#define __ORTHANC_HASH_H__
#include <uthash.h>
#include <string.h>

#define MAX_CHAR_HASH 200
#ifndef __PERSO_MALLOC__
#define __PERSO_MALLOC__
#include <stdlib.h>
#define Malloc(type,n) (type *)malloc((n)*sizeof(type))
#endif

struct opair{
	char rname[200];
	int o_id;
	UT_hash_handle hh;
};
//Global hash tables
struct opair * hash_patient = NULL;
struct opair * hash_serie = NULL;
struct opair * hash_study = NULL;
struct opair * hash_instance = NULL;

/*
	The ids that are supposed to be get are the ids coming from the filesystem.
	These ids are thus supposed to be unique:
	ex: /bobama/heartirm/shortaxis/
		/bobama/heartirm_1/shortaxis/
	are valid identifiers for two different dicom studies having names heartirm
	
	An id is supposed to be predecessed by the values of its predecessors in the
	hierarchy.
	ex: for the shortaxis of the second heart irm of bobama, we search
	/bobama/heartirm_1/shortaxis
	instead of
	shortaxis
	The same goes for the orthanc identifiers
*/
void add_patient(char * ofs_patient_name, int o_pid);
void add_study(char * ofs_study_name, int o_stid);
void add_serie(char * ofs_serie_name, int o_seid);
void add_instance(char * ofs_instance_name, int o_iid);


/*
	Again, the ofs ids are predecessed by its predecessors in the hierarchy.
	The orthanc ids returned will also be in that way.
*/
int get_o_pid(char * ofs_pid);
int get_o_stid(char * ofs_stid);
int get_o_seid(char * ofs_seid);
int get_o_iid(char * ofs_iid);
#endif
