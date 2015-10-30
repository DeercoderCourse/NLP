ABOUT THE IMDB FILM GENRE CORPUS
--------------------------------

The files in this corpus are for personal use only; they were created from 
the plaintext files available here: http://www.imdb.com/interfaces and 
used in accordance with IMDB's data use policies.  All information 
about the movies and summaries belongs to IMDB.

The IMDB corpus was created by randomly selecting movie summaries from the 
IMDB database that met certain criteria.  These criteria were:

* The item the summary is about must not be a video games, TV show or 
TV show episode, or a made-for-TV movie.  Direct-to-DVD movies were 
included in the corpus

* The summary had to contain at least 200 words

The movies for the corpus were selected randomly as follows.  For each
of the chosen 10 categories (Action, Adventure, Comedy, Drama,
Fantasy, Mystery, Romance, Sci-Fi, Thriller, Western), we randomly
selected 35 movies labeled with that genre and 35 movies not labeled
with that genre.

Note that since movies frequently have multiple genre labels, negative
as well as positive examples in each category typically overap.  What's
guaranteed is that for every category, the corpus contains 35 positive
examples and 35 negative examples.

_____________________________________________________________________

CONTENT WARNING
---------------

The movies and files in this database were selected randomly and were not 
screened to determine if the movie summaries contain adult language or 
describe adult situations.  Read at your own risk.

_____________________________________________________________________


SUMMARY OF INCLUDED MATERIALS
-----------------------------

The imdb_corpus folder contains two directories: 
    imdb_corpus/records
    imdb_corpus/labels

The records directory contains the files that are intended for training 
and testing ML algorithms.  

The labels folder contains the information about the files in the
records directory.  It has three subdirectories:

    labels/all
    labels/training
    labels/testing

Each of these directories contains, for each genre category, a list of
positive instances (positive_<GenreName>.txt) and a list of negative
instances (negative_<GenreName>.txt).  

labels/all 
   - all records, 35 positive and 35 negative examples per category
labels/training 
   - training records, 25 positive and 25 negative examples per category
labels/testing 
   - test records, 10 positive and 10 negative examples per category

For example, for the Action category, the labels/training/ directory
contains two files: positive_Action.txt and negative_Action.txt.

The file positive_Action.txt contains the titles of the movies in the
Action category, as well as the full list of genres assigned to each
title in IMDB, separate by the pipe ('|') symbol.  Here is an example
line from the file:

Renegades of Sonora (1948)|Action|Western

Similarly, the file negative_Action.txt contains the titles of the 
movies that are NOT in the Action category, similarly formatted.

NOTE: the genres listed for each movie include *all* of the genres
that IMDB has listed for that movie, not just the ones selected for
this corpus.  So labels like Animation, Family, etc. appear in the
files, even though these categories are not a part of this corpus.

_____________________________________________________________________

ACKNOWLEDGEMENTS
---------------

This corpus was compiled for the 91.530.201 Special Topics: Natural
Language Processing class taught by Anna Rumshisky at the Dept. of
Computer Science at UMass Lowell (Fall 2012).  The scripts used to
compile the corpus were created by Amber and Royce Stubbs and modified
by Anna Rumshisky and Amber Stubbs.

