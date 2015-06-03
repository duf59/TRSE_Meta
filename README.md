# Optimizing a Textual Search Engine with MeTa data sciences toolkit

This competition was part of the "Text Retrieval and Search Engine" class given by [professor ChengXiang Zhai](http://czhai.cs.illinois.edu/) on Coursera (University of Illinois - Dpt of Computer Science). Objective was to implement and optimize a search engine using the [MeTA toolkit](https://meta-toolkit.org/). MeTa is a C++ data sciences toolkit that supports a variety of tasks such as text retrieval, natural language processing, and machine learning.https://meta-toolkit.org/

For the competition we used a dataset containing the description of about 23.000 MOOCs.
I implemented and tuned different vector space model ranking functions, including Okapi BM25, jelinek-mercer, absolute-discount, etc. along with modified rankers proposed in [1][1]. The best results were obtained with a modified Pivoted-Length ranker (MPtf2ln) from [1][1], with which I ranked #22 over 200+ participants on the leaderboard. The codes requier basic understanding of search engine working principle, especially vector space model ranking function, along with basic knowledge of C++.

This report describes the prerequisites, the dataset used, and the strategies I implemented to optimize the search engine.

The repo contains the following files:

* `data/`: the dataset
* `Assignment_2.tar.gz`: source files for the competition. After installing MeTa, you have to decompress it and run the python script inside if you want to run my codes.
* `bm25 tuning/`: results from using okapi bm25 ranker and R code for the analysis
* `mdtf2ln tuning`: results from using modified dirichlet ranker and R code for the analysis
* `mptf2ln tuning`: results from using modified pivoted length ranker and R code for the analysis
* `R bag of word`: R code to analyze the dataset and find top N words using bag of word model
* `competition.cpp`: the main program use for the competition
* `src/`: different version of main programme *competition.cpp*, implementing different rankers and rocchio pseudo feedback (MeTa needs to be recompiled with these files renamed to competition.cpp and put in meta/src/index/tools before use)

[1]: H. Fang, T. Tao, C. Zhai, Diagnostic Evaluation of Information Retrieval Models, 2010

## Prerequisites

Software prerequisites are:

* MeTA
* Python 2.7 (used only for setting up the assignment files, not for coding)

MeTA officially supports Linux operating systems and Mac OS X 10.6 or higher but does not support Windows. To install it go to the [MeTA Setup Guide](https://meta-toolkit.github.io/meta/setup-guide.html) and follow the steps specific to your operating system. 

Important note: after cloning the MeTa repo, I advise to reset to version 1.3.2 (all the present codes were run under this version). Use `git reset --hard v1.3.2`. Then proceed with the rest of the installation instructions on the web page.

You can read [MeTA's System Overview](https://meta-toolkit.github.io/meta/overview-tutorial.html) to gain a high level understanding of how MeTA operates (this is beyond the scope of this report!). To know more about a certain class or function, you can use the [MeTA's Documentation](https://meta-toolkit.github.io/meta/doxygen/namespaces.html), which provides a brief explanation of the different modules.

If you want to run the codes provided in this repo you will also have to uncompress *Assignment_2.tar.gz* to the directory where meta is located. Then run the python script inside. This will copy the dataset into meta/data folder along with several other files necessary to run the main code of this competition.

## Dataset

The MOOCs dataset contains the descriptions found on the webpages of around 23,000 MOOCs (Massive Open Online Courses).

* `moocs.dat`: contains the content of the webpages of all the MOOCs; each MOOC's main page occupies exactly one line in the file.
* `moocs.dat.names`: contains the names and the URLs of the MOOCs.
* 'moocs-queries.txt': contains the training and testing queries used to build the search engine(*training queries are the first 100 entries*)
* `moocs-qrels.txt`: contains the relevance judgments corresponding to the queries in moocs-queries.txt. Each line in moocs-qrels.txt has the following format: (querynum documentID 1). This means that the document represented by documentID is a relevant document for the query whose number is querynum. The relevance judgments in moocs-qrels.txt were created by human assessors who ran the queries and chose the relevant documents. We use these judgements to quantify the performance of the search engine on the training set.

## Indexing

Before implementing and optimizing the search engine, the first step is to index the MOOCs dataset. In this process, an inverted index is created. Parameters for indexing are specified in the configuration file config.toml (from the build directory). For instance, the snippet shown below tells the indexer where to find the MOOCs dataset, specifies that it is a line corpus (i.e., each document is on one line), and defines the name of the inverted index to be created. The forward index is used for basic text retrieval (although it is necessary for pseudo relevance feedback such as rocchio feedback).

```
query-judgements = "../data/moocs/moocs-qrels.txt"
querypath = "../data/moocs/"
corpus-type = "line-corpus"
dataset = "moocs"
forward-index = "moocs-fwd"
inverted-index = "moocs-inv"
```

Another important snippet for indexing is:

```
[[analyzers]]
method = "ngram-word"
ngram = 1
filter = "default-chain"
```

The settings under the analyzers tag control how tokenization and filtering are performed, prior to creating the inverted index. By default, method is "ngram-word" and ngram=1, which means that the tokenizer will segment each document into unigrams (i.e., single words). filter="default-chain" tells MeTA to use its default filtering chain, which performs a couple of predefined filters including lower case conversion, length filtering (which discards tokens whose length is not within a certain range), and stemming. To read more about modifying META's default tokenization and filtering behavior see [MeTA’s Analyzers and Filters page](https://meta-toolkit.github.io/meta/analyzers-filters-tutorial.html).

To index the dataset, run:

```
cd ~/Desktop/meta/build/
./index config.toml
```

This will start by performing tokenization and applying the text filters defined in config.toml; it then creates the inverted index and places it in /meta/build/moocs-inv. The process will take some time to finish as it is indexing a large number of documents. When the program finishes execution, you should get a summary of the indexed corpus (number of documents, average document length, etc.).

## Building the search engine

The main program for the competition is `competition.cpp` which must be located and excecuted from meta/src/index/tools/. In the main() function, first loop passes over the 100 training queries and prints the precision at 10 documents and the mean average precision (MAP). The second while loop passes over the 538 testing queries and writes the IDs of the top 50 documents corresponding to each query to the output file output.txt in meta/build/Assignment2/ (this file is used for submission).

I provide some of the modified version of the program implementing different rankers and rocchio pseudo feedback. If you want to run them, you should copy the file in meta/src/index/tools/ and rename it as competition.cpp (i.e. replace the original file). Then rebuild MeTa.

## Optimizing the search engine

The first step has been to focus on MeTa built in rankers. Keeping the default filter chain for the analyzer, the following rankers were tested (with corresponding MAP on the test set):

* Okapi bm25
* absolute-discount
* dirichlet-prior
* jelinek-mercer
* pivoted-length

I tuned the rankers by varying their parameters over a wide range of values.
I exported the data (parameter values and MAP) to text files and used R to plot the mean average precision (MAP) and decide on the optimal parameters values. Plotting the MAP also give an idea of the ranker robustness (i.e. shape of the curves). From this first analysis, bm25 and jelinek-mercer appeared as the most promising (reaching MAP on the test data of about 0.622, reaching the top 5 during the first week of competition). See the data in folder bm25_tuning for an example, it contains the output of bm25 tuning and the R script to plot the map curves.

A second approach has been to modify the analyzer filter chain (used to index the corpus).
Especially I changed the minimum word length (length_filter min parameter) from 2 to 1.
This allowed a slight improvement of the Test MAP, reaching about 0.627, due to some query related to programming language like R or C, which were not matched before.

Third step was to implement and tune other rankers :

* pl2
* MPtf2ln (modified Pivoted-length from [1][1])
* MDtf2ln (modified Dirichlet from [1][1])

The pl2 ranker appeared not very efficient (Train MAP up to 0.58).
MPtf2ln and MDtf2ln gave good results and I achieved my best score with MPtf2ln reaching a MAP on the test data of 0.6317. The latter ranker also appeared to be quite robust (MAP curves are relatively flat with the optimum not strongly sensitive to slight change in parameters). So I decided to keep this ranker for the rest of my investigations.

data from MPtf2ln and MDtf2ln tuning are provided in the corresponding folders, along with the R script to plot the MAP curves.

## Other strategies

Besides the approaches discussed above, I tried the following, which however was unsuccessfull:

* Implementation of Rocchio pseudo Feedback (the idea is to assume the top N retrieved documents are relevant, and augment the query based on those documents). This did not improve the search engine eficiency.
* Analysis of the words distribution (both in query and mooc datasets), and adding most common words such as "learn", "course" or "data" to the stopwords list. (I performed this bag of word analysis with R using package "tm"). This also did not improve the search engine efficiency.

## Perspectives

From the discussions on the forum it seems that mixing the two approaches discussed above (pseudo feedback + new stop words) can lead to better results. Also, best scores on the leaderboard were obtained by using ensembles, i.e. weighted average of different rankers. It is known that ensemble are rather powerfull in machine learning and it seems to be similar for vector space model based ranking.

