OrthancFS
=========

Orthanc fuse filesystem (trial)

This project aims to provide a read-only filesystem to the Orthanc project
(code.google.com/p/orthanc/). Because of fuse, it is only supported on 
Linux-based machine. The underlying goal was to be able to load Orthanc images 
into visualization and processing softwares like 3D Slicer (www.slicer.org).

1. Installation

Some adjustments will probably be needed:

1) First, you have to build Orthanc (laaw branch). One .so file is built during
the process. You have to set its location (use static path if possible) in the
include/orthancfs.h directory (line 13).

For this, you need to: 
	clone the orthanc repository:
	$hg clone https://code.google.com/p/orthanc/
	get the laaw branch:
	$hg update -c laaw
	then compile it (using cmake) according to your configuration	

2) Then you need to give the place of the corresponding header. For now, the
header present in include/OrthancClient.h is enough but will likely change along
the Orthanc versions.

3) The fuse development packages need to be installed

4) Congratulations! you can now type 
	$make

5) usage example:
	$mkdir orthanc_mount
	$./ofs http://localhost:8042 orthanc_mount/
	
	where the first argument is ALWAYS the orthanc adress, and the last is the
	mount point. In-between are the fuse arguments.
	
NOTE : OrthancFS will always be mounted with the fuse option "single thread",
even if you do not specify it.
