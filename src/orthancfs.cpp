#include "../include/orthancfs.h"
#include <string.h>
#include <map>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>

/*
	TODO:
		- speed problem
		- name simplification for compatibility with slicer?		
*/
using namespace std;

/**
	Get the type of the path given.
	There are five accepted types of paths in OrthancFS:
	/ is the root
	/patient represents the patient with name patient
	/patient/study represents the study "study" of the patient with name patient
	/patient/study/series represents the series "series" with ...
	/patient/study/series/0000i.dcm represents the instance n°i of ...
	
	EVERYTHING ELSE IS IGNORED (or should be...)
	
	@PRE:path is a valid string (ends with \0)
	@POST:\
	@RETURN: the type of the string (see header)
*/
int get_ofs_type(const char * path){
	int l = strlen(path);
	int n = 0;
	if(l==1 && path[0]=='/')return OFS_ROOT;
	for(int i=0;i<l-1;i++){
		if(path[i]=='/')
			n++;
	}
	if(n==1)return OFS_PATIENT;
	else if(n==2)return OFS_STUDY;
	else if(n==3)return OFS_SERIES;
	else if(n==4) return OFS_INSTANCE;
	else return OFS_ERROR;
}



/**
	From a random valid path, this function returns a subpart of it: it allows
	to get the patient of a series,...
	
	@PRE:path is valid string
	@POST:\
	@RETURN: substring of the path, up until max_slash+1 / are encountered
			 the last / is not returned
*/
char * get_helper(const char * path, int max_slash){
	int l = strlen(path);
	int n=1;
	int n_slash = 0;
	char * dst;
	while(n<l){
		if(path[n]=='/')n_slash++;
		if(n_slash>=max_slash)break;
		n++;
	}
	dst = (char*)malloc((n+1)*sizeof(char));
	strncpy(dst,path,n);
	dst[n]='\0';
	return dst;
}

char * get_patient(const char * path){
	return get_helper(path,1);
}

char * get_study(const char * path){
	return get_helper(path,2);
}

char * get_series(const char * path){
	return get_helper(path,3);
}

/**
	OrthancFS initializer:
		loads an OrthancClient::OrthancConnection to the class.
	@PRE:\
	@POST: the orthanc connection is initialized if the orthanc address is valid
		   otherwise an exception is thrown
	@RETURN:\
*/
OrthancFS::OrthancFS(const char * address){
	try{
    	OrthancClient::Initialize(LIBORTHANCADRESS);
    	orthanc = new OrthancClient::OrthancConnection(address);
    }
    catch (OrthancClient::OrthancClientException e){
    	cerr << "EXCEPTION: [" << e.What() << "]" << std::endl;
  	}
}

//Destructor : destroys the Orthanc connection object
OrthancFS::~OrthancFS(){
	delete orthanc;
}

/**
	Returns the id of tha patient/study/series/instance (see header)
	@PRE: the path is a valid identifier present in the corresponding table
	@POST:\
	@RETURN: the corresponding identifier
	TODO
		Remove the @pre!
*/
int OrthancFS::get_o_pid(const char * path){
	string s(path);
	return map_patient[s];
}

int OrthancFS::get_o_stid(const char * path){
	string s(path);
	return map_study[s];
}

int OrthancFS::get_o_seid(const char * path){
	string s(path);
	return map_series[s];
}

int OrthancFS::get_o_iid(const char * path){
	string s(path);
	return map_instance[s];
}

/** 
	Fuse function for setting the attributes of a file
	@PRE: path is a valid path
	@POST: stbuf contains informations about the file/repository:
		-> read mode and number of links for repository
		-> read mode, nlink, and size for file
	@RETURN: 0 if the file was ok
			-ENOENT otherwise
	TODO:
		1) get_ofs_type does not check the validity well, it only counts the /
		do a better validity check
		2) add ownership info
		3) check the read/write rights
*/
int OrthancFS::getAttr(const char *path, struct stat *stbuf){
	int type = get_ofs_type(path);
	int p_index,st_index,se_index,i_index;
	char * p_name,*st_name,*se_name;
	memset(stbuf, 0, sizeof(struct stat));
	
	stbuf->st_uid = getuid();
	stbuf->st_gid = getgid();
	if(type==OFS_ROOT){
		stbuf->st_mode = S_IFDIR | 0444;
		stbuf->st_nlink = orthanc->GetPatientCount()+2;
	}
	else if(type==OFS_PATIENT){
		if(map_patient.find(path)==map_patient.end()){
			return -ENOENT;
		}
		p_index = get_o_pid(path);
		OrthancClient::Patient patient = orthanc->GetPatient(p_index);
		stbuf->st_mode = S_IFDIR | 0444;
		stbuf->st_nlink = patient.GetStudyCount()+2;
	}
	else if(type==OFS_STUDY){
		if(map_study.find(path)==map_study.end()){
			return -ENOENT;
		}
		p_name = get_patient(path);
		p_index = get_o_pid(p_name);
		OrthancClient::Patient patient = orthanc->GetPatient(p_index);
		st_index = get_o_stid(path);
		OrthancClient::Study study = patient.GetStudy(st_index);
		stbuf->st_mode = S_IFDIR | 0444;
		stbuf->st_nlink = study.GetSeriesCount()+2;
		free(p_name);
	}
	else if(type==OFS_SERIES){
		if(map_series.find(path)==map_series.end()){
			return -ENOENT;
		}
		p_name = get_patient(path);
		st_name = get_study(path);
		p_index = get_o_pid(p_name);
		st_index = get_o_stid(st_name);
		se_index = get_o_seid(path);
		OrthancClient::Patient patient = orthanc->GetPatient(p_index);
		OrthancClient::Study study = patient.GetStudy(st_index);
		OrthancClient::Series series = study.GetSeries(st_index);
		stbuf->st_mode = S_IFDIR | 0444;
		stbuf->st_nlink = series.GetInstanceCount()+2;
		free(p_name);
		free(st_name);
	}
	else if(type==OFS_INSTANCE){
		if(map_instance.find(path)==map_instance.end()){
			return -ENOENT;
		}
		p_name = get_patient(path);
		st_name = get_study(path);
		se_name = get_series(path);
		p_index = get_o_pid(p_name);
		st_index = get_o_stid(st_name);
		se_index = get_o_seid(se_name);
		i_index = get_o_iid(path);
		OrthancClient::Patient patient = orthanc->GetPatient(p_index);
		OrthancClient::Study study = patient.GetStudy(st_index);
		OrthancClient::Series series = study.GetSeries(se_index);
		OrthancClient::Instance instance = series.GetInstance(i_index);
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = instance.GetDicomSize();
		free(p_name);
		free(st_name);
		free(se_name);
	}
	else
		return -ENOENT;
	return 0;
}

/**
	Trick function to avoid going into .Trash repositories
	TODO
		Not use this horror!
*/
/*bool trash(const char * path){
	return (path[0]=='/' && path[1] == '.' && (path[2] =='T' || path[2] == 't'));
}*/

/**
	The goal of this function is to fill buf with the strings of the repositories
	present in path.
	For example, if the path is /patient/, we will fill buff with all the
	studies string of the patient
	@PRE:\
	@POST: buf is filled with the repositories to read.
		The patient of orthanc
		The studies of the patient
		The series of the studies
		The instance of the series
		The corresponding hashtables are updated (see the header)
	@RETURN: 0 if this is an orthanc directory, (TODO error otherwise)
*/
int OrthancFS::readDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
	int type = get_ofs_type(path);
	if(type==OFS_INSTANCE){
		return -ENOTDIR;
	}
	int i,j,n,p_id,st_id,se_id;
	string st,cur;
	stringstream ss;
	char * p, *stu,*ser;
	filler(buf,".",NULL,0);
	filler(buf,"..",NULL,0);
	j=1;
	if(type==OFS_ROOT){
		orthanc->Refresh();
		n = orthanc->GetPatientCount();
		map_patient.clear();
		for(i=0;i<n;i++){
			OrthancClient::Patient p = orthanc->GetPatient(i);
			st = p.GetMainDicomTag("PatientName","B. Obama");
			cur = st;
  			while(map_patient.find("/"+cur)!=map_patient.end()){
  				ss.str(string());
  				ss << j;
  				cur = st+" ("+ss.str()+")";
  				j++;
  			}
  			map_patient["/"+cur]=i;
  			filler(buf,cur.c_str(),NULL,0);
		}
	}
	else if(type==OFS_PATIENT){
		if(map_patient.find(path)==map_patient.end()){
			return -ENOENT;
		}
		map_study.clear();
		p = get_patient(path);
		p_id = get_o_pid(p);
		OrthancClient::Patient pat = orthanc->GetPatient(p_id);
		pat.Reload();
		n = pat.GetStudyCount();
		string pat_string = string(path)+"/";
		string cur;
		for(i=0;i<n;i++){
			OrthancClient::Study stu = pat.GetStudy(i);
			st = stu.GetMainDicomTag("StudyDescription","Medical Data : Study");
			cur = st;
			while(map_study.find(pat_string+cur)!=map_study.end()){
				ss.str(string());
				ss<<j;
				cur = st + " (" + ss.str() +")";
				j++;
			}
			map_study[pat_string+cur]=i;
			filler(buf,cur.c_str(),NULL,0);
		}
		free(p);
	}
	else if(type==OFS_STUDY){
		if(map_study.find(path)==map_study.end()){
			return -ENOENT;
		}
		map_series.clear();
		p = get_patient(path);
		p_id = get_o_pid(p);
		stu = get_study(path);
		st_id = get_o_stid(stu);
		OrthancClient::Patient pat = orthanc->GetPatient(p_id);
		OrthancClient::Study stud = pat.GetStudy(st_id);
		stud.Reload();
		n = stud.GetSeriesCount();
		string pat_string = string(path)+"/";
		string cur;
		for(i=0;i<n;i++){
			OrthancClient::Series ser = stud.GetSeries(i);
			st = ser.GetMainDicomTag("SeriesDescription","Medical Data : Study");
			cur = st;
			while(map_series.find(pat_string+cur)!=map_series.end()){
				ss.str(string());
				ss<<j;
				cur = st + " (" + ss.str() +")";
				j++;
			}
			map_series[pat_string+cur]=i;
			filler(buf,cur.c_str(),NULL,0);
		}
		free(p);
		free(stu);
	}
	else if(type==OFS_SERIES){
		if(map_series.find(path)==map_series.end()){
			return -ENOENT;
		}
		map_instance.clear();
		p = get_patient(path);
		p_id = get_o_pid(p);
		stu = get_study(path);
		st_id = get_o_stid(stu);
		ser = get_series(path);
		se_id = get_o_seid(ser);
		OrthancClient::Patient pat = orthanc->GetPatient(p_id);
		OrthancClient::Study stud = pat.GetStudy(st_id);
		OrthancClient::Series series = stud.GetSeries(se_id);
		series.Reload();
		n = series.GetInstanceCount();
		string pat_string = string(path)+"/";
		string cur;
		for(i=0;i<n;i++){
			OrthancClient::Instance ins = series.GetInstance(i);
			string inumber = ins.GetTagAsString("InstanceNumber");
			inumber = string(5-inumber.length(),'0').append(inumber)+".dcm";
			map_instance[pat_string+inumber]=i;
			filler(buf,inumber.c_str(),NULL,0);
		}
		free(p);
		free(stu);
		free(ser);
	}
	return 0;
}

/**
	Check if the file can be opened.
	@PRE:\
	@POST: if the repository is not on the hashmap, it does not come from
	orthanc so it can not be opened.
	@RETURN: -ENOENT if the file does not exist
			 0 if it does
*/
int OrthancFS::open(const char *path, struct fuse_file_info *fi){
	int type = get_ofs_type(path);
	if(type==OFS_ROOT)return 0;
	else if(type==OFS_PATIENT){
		if(map_patient.find(path)!=map_patient.end())return 0;
		else return -ENOENT;
	}
	else if(type==OFS_STUDY){
		if(map_study.find(path)!= map_study.end())return 0;
		else return -ENOENT;
	}
	else if(type==OFS_SERIES){
		if(map_study.find(path)!=map_series.end())return 0;
		else return -ENOENT;
	}
	else if(type==OFS_INSTANCE){
		if(map_instance.find(path)!=map_instance.end())return 0;
		else return -ENOENT;
	}
	return 0;	
}


/**
	Read a file.
	@PRE: \
	@POST: buf is filled with size octets of data of path
	@RETURN: size if it was possible to fill buf with size octets
			 0 if it was not possible to read anything
			 or the amo data loaded in buf
*/
int OrthancFS::read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi){
	int type = get_ofs_type(path);
	if(type!=OFS_INSTANCE){
		return -EISDIR;
	}
	else{
		if(map_instance.find(path)==map_instance.end())return -EBADF;
	}
	char * pat = get_patient(path);
	char * stu = get_study(path);
	char * ser = get_series(path);
	const char * ins = path;
	int pat_id = get_o_pid(pat);
	int stu_id = get_o_stid(stu);
	int ser_id = get_o_seid(ser);
	int ins_id = get_o_iid(ins);
	OrthancClient::Patient o_pat = orthanc->GetPatient(pat_id);
	OrthancClient::Study   o_stu = o_pat.GetStudy(stu_id);
	OrthancClient::Series  o_ser = o_stu.GetSeries(ser_id);
	OrthancClient::Instance o_ins = o_ser.GetInstance(ins_id);
	
	char * buffer = (char*) o_ins.GetDicom();
	size_t len = o_ins.GetDicomSize();
	if((unsigned int)offset<(unsigned int)len){
		if(offset+size>len)
			size = len-offset;
		memcpy(buf, buffer + offset, size);
	}
	else{
		size = 0;
	}
	
	free(pat);
	free(stu);
	free(ser);
	return size;
}

int OrthancFS::access(const char * path, int mode){
	int type = get_ofs_type(path);
	
	if(mode==2 || mode==6 || mode==3)
		return -EACCES;
	if(type==OFS_ROOT)
		return 0;
	else if(type==OFS_PATIENT){
		if(map_patient.find(path)==map_patient.end())
			return -ENOENT;
		return 0;
	}
	else if(type==OFS_STUDY){
		if(map_study.find(path)==map_study.end())
			return -ENOENT;
		return 0;
	}
	else if(type==OFS_SERIES){
		if(map_series.find(path)==map_series.end())
			return -ENOENT;
		return 0;
	}
	else if(type==OFS_INSTANCE){
		if(map_instance.find(path)==map_instance.end())
			return -ENOENT;
		return 0;
	}
	return -ENOENT;
}


/**
	C-like functions to call the OrthancFS object in the structure. Not sexy
	but working!
*/
int ofs_getAttr(const char *path, struct stat *stbuf){
	OrthancFS * ofesse = OFS_DATA->ofs;
	return ofesse->getAttr(path,stbuf);
}

int ofs_readDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
	OrthancFS * ofesse = OFS_DATA->ofs;
	return ofesse->readDir(path,buf,filler,offset,fi);
}

int ofs_open(const char *path, struct fuse_file_info *fi){
	OrthancFS * ofesse = OFS_DATA->ofs;
	return ofesse->open(path,fi);
}

int ofs_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi){
	OrthancFS * ofesse = OFS_DATA->ofs;
	return ofesse->read(path,buf,size,offset,fi);
}

int ofs_access(const char * path, int mode){
	OrthancFS * ofesse = OFS_DATA->ofs;
	return ofesse->access(path,mode);
}

/**
	Core structure.
	TODO Check if other functions could be interesting
*/
struct hello_fuse_operations:fuse_operations
{
    hello_fuse_operations ()
    {
        getattr    = ofs_getAttr;
        readdir    = ofs_readDir;
        open       = ofs_open;
        read       = ofs_read;
        access     = ofs_access;
    }
};

static struct hello_fuse_operations hello_oper;

void ofs_usage(char * l){
	cout << "OrthancFS should be used like this:" << endl;
	cout << l << " [mount options] orthancadress:listening_port mounting_point" << endl;
	cout << "example : " << endl;
	cout << l << " http://localhost:8042 mounted_orthanc/" << endl;
}

int main(int argc, char *argv[]){
	OrthancFS * ofes = NULL;
	int fuse_stat;
	if ((getuid() == 0) || (geteuid() == 0)) {
		fprintf(stderr, "Running OrthancFS as root opens unnacceptable security holes\n");
		return EXIT_SUCCESS;
    }
    
	if(argc<3){
		ofs_usage(argv[0]);
		return EXIT_SUCCESS;
	}
	
	try{
		ofes = new OrthancFS(argv[argc-2]);
	}
	catch (OrthancClient::OrthancClientException e){
    	cerr << "EXCEPTION: [" << e.What() << "]" << endl;
    	ofs_usage(argv[0]);
  		return EXIT_SUCCESS;
  	}
  	
  	argv[argc-2] = (char*)"-s";
  	
	struct ofs_state * ostate = (struct ofs_state*) malloc(sizeof(struct ofs_state));
	if(ostate==NULL){
		cerr << "malloc error" << endl;
		return EXIT_SUCCESS;
	}
	
	ostate->ofs = ofes; 
	fuse_stat = fuse_main(argc, argv, &hello_oper, ostate);
	
	delete ofes;
	free(ostate);
	return fuse_stat;
}
