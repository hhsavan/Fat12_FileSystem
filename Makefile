all:	
	g++ -o makeFileSystem makeFileSystem.cpp
	g++ -o fileSystemOper fileSystemOper.cpp
	
clean:
	rm -f makeFileSystem fileSystemOper
