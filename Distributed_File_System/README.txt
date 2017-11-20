#Distributed File System
##Network Systems Fall-2017
###Made By: Mukund Madhusudan Atre


Instructions to run the Distributed File System

	1. Compile the dfc.c (make)
	2. Compile the dfs.c (make)
	3. Open a terminal and type "./dfc dfc.conf".
	4. Open a terminal and type "./dfs DFS1 10001 &",  "./dfs DFS2 10002 &",  "./dfs DFS3 10003 &",  "./dfs DFS4 10004 &",
    5. Run the commands in client terminal:
        a. LIST <sub-folder>
        b. GET <filename> <sub-folder>
        c. PUT <filename> <sub-folder>
        d. MKDIR <directory-name>

        The <sub-folder> in the command is optional


Features Implemented
    1. Advanced GET recognizes all the pieces of a file present on the Servers that are online.
    2. PUT splits a file and distributes it onto 4 servers according to the modulus of md5sum.
    3. LIST lists all the files present on the DFS and indicates if parts are enough to reconstruct the file again.
    4. MKDIR creates the requested directory on the DFS.

Extra Credit Work
    Enhanced GET optimizes traffic by requesting one part of a file only once.

References:
    1. https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_71/rzab6/xnonblock.htm
    2. http://clinuxcode.blogspot.com/2014/02/concurrent-server-handling-multiple.html
    3. http://developerweb.net/viewtopic.php?id=3196
    4. https://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c
