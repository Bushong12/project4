# project4
Cse 30141 Project IV: Web Placement System
Grace Bushong (gbushong)
Erin Turley (eturley)

To run our program, compile using "make", then run with ./main inputfilename.txt

Our program's main function instantiates a Config object and uses that object to parse the various files (input file, search file, site names file). In a while loop that restarts every number of seconds specified in the input file, it creates the number of fetch and parse threads also specified by the input file. The fetch threads are sent to a function called get_site_name, while the parse threads are sent to a function called find_words.

When the fetch threads are in the function get_site_name, they must wait for the queue of site names to be populated (by a function called in main). This is handled with a condition variable-- when the queue of site names is populated, it broadcasts to the fetch threads waiting. These threads can individually pop site names from the queue and call get_site, passing in the name of the site as a parameter. get_site uses the libcurl code that I essentially copied from the site's examples. The only difference is that it uses another condition variable-- this time, each site's data is atomically pushed to a queue called queue_data. Once there is data in this queue, it broadcasts to the parse threads waiting on it.

The parse threads start in the function find_words. Once the queue_data is populated in get_site by the fetch threads, it pops the data off (stored as a pair to save the site name and the site data together) and searches the data for each word specified in the search file. All of this is done atomically, within a mutex lock. Once all of the words have been found for one site, that data can be written to the output file, so it broadcasts to a condition variable in the function write_to_output. In write_to_output, it first waits for a site's search words and word counts to be printed. Once the queue_word_counts is populated, it begins writing to the output file and pops elements off of the queue atomically. 