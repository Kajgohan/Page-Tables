Ken Desrosiers & Jack Hogan

The hardest part about this project was conceptually understanding everything that was supposed to happen. Knowing what the page table was for and how it worked, as well as the swapping were the most difficult parts of this project.
Once we understood everything, the project went pretty smoothly overall.

Things to keep in mind while grading:
When using map, there are some instances where the program will output the string, "Virtual page %d already has these permissions!". This message is intended to warn the user that the input they gave covers the same process, virtual
page, and protection bit. This message, however, may also display when creating a new page table and virtual address space though swapping. This is not an error, but it is a result of the recursion we are using. Trying to build
another case that avoids this would be troublesome, so we figured it wouldn't be a very big deal. In order to run, make sure that the makefile, C file, and disk,txt are all in the same directory. Run the command 'make', and then run
the command './page'. This will run the program. Included are also some test cases that we ran as well as their output files. To run the test cases, just run ./page < 'test.txt'. Note that the segmentation fault at the end of the test when you run it is normal because the EOF is not is the correct format specififies by scanf, so the program stops.

The first case covers the basics of map, store, and load.
The second test case covers if the user loads a virtual address space from disk
The third test case covers if the user loads a virtual address space as well as its page table from disk.
The fourth test case covers all of the errors and warnings the user could get.


Thank you,
	
	Ken & Jack