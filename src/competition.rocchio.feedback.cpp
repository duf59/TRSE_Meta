/**
 * @file competition.cpp
 * @author Hussein Hazimeh
 * Built based on query-runner.cpp which is written by Sean Massung
 */
#include <vector>
#include <string>
#include <iostream>
#include "index/eval/ir_eval.h"
#include "util/time.h"
#include "corpus/document.h"
#include "index/inverted_index.h"
#include "index/ranker/ranker_factory.h"
#include "parser/analyzers/tree_analyzer.h"
#include "sequence/analyzers/ngram_pos_analyzer.h"
#include <cmath>
#include "index/score_data.h"
#include <random>
#include "index/forward_index.h"
#include "index/postings_data.h"

using namespace meta;

/*
class new_ranker: public index::ranker
{
private: // Change the parameters to suit your ranking function
    double param1_ = 0;
    double param2_ = 0;

public:
    const static std::string id;
    new_ranker(); // Default Constructor
    new_ranker(double param1, double param2); // Constructor
    void set_param(double param1, double param2){param1_ = param1; param2_ = param2;}; // Sets the parameters
    double score_one(const index::score_data&); // Calculates the score for one matched term
};

const std::string new_ranker::id = "newranker"; // Used to identify the ranker in config.toml
new_ranker::new_ranker(){}
new_ranker::new_ranker(double param1, double param2) : param1_{param1}, param2_{param2} {}

double new_ranker::score_one(const index::score_data& sd)
{
    // Implement your scoring function here

   return 0;

}


namespace meta{
namespace index{
template <>
std::unique_ptr<ranker>make_ranker<new_ranker>(
        const cpptoml::table & config) // Used by new_ranker to read the parameters param1 and param2 from config.toml
{
    double param1 = 0; // Change to the default parameter value
    if (auto param1_file = config.get_as<double>("param1"))
        param1 = *param1_file;

    double param2 = 0; // Change to the default parameter value
    if (auto param2_file = config.get_as<double>("param2"))
        param2 = *param2_file;

    return make_unique<new_ranker>(param1, param2);
}

}
}

*/


corpus::document Rocchio(index::inverted_index& idx, index::forward_index& fidx, corpus::document &query, std::vector<std::pair<long unsigned int,double>> &ranking)
{
    double alpha(1.0), beta(0.1); // weights
    unsigned int threshold(10);
    corpus::document new_query; // the new query
    // std::cout << "Rocchio function. Doc 1 ID : " << ranking[1].first << std::endl;
    // std::cout << "Rocchio function. Doc 1 count : " << ranking[1].second << std::endl;
    double numdoc = ranking.size();

    //idx.tokenize(query); // do no do it here because it double the counts

    // STEP 1 JUST COPY THE QUERY INTO THE NEW_QUERY

    // Add the terms of the original query to the new query
    for (auto& qpair : query.counts())
    { // Loop over the original query's  pairs
    std::cout << "Adding new term from original query : " << qpair.first << " with count " << qpair.second << std::endl;
    new_query.increment(qpair.first,alpha*qpair.second); // qpair.first is the term name and qpair.second is the term frequency
    }

    // add terms from docs
    for (size_t i = 0; i < ranking.size(); ++i) // Loop over the top-ranked documents in ranking
    {
        auto postings_list = fidx.index::forward_index::search_primary(ranking[i].first); // Return the postings list of the document

        auto term_freq = postings_list->counts(); // term_freq contains the pairs of the form <term_id, term_freq>

        // Add the terms of the document to the new query
        for (auto& tfpair : term_freq)
        {
            if (tfpair.second>threshold)
            {
                std::cout << "Adding new term from doc["<< i << "] : " << idx.term_text(tfpair.first) << " with count " << tfpair.second << std::endl;
                new_query.increment(idx.term_text(tfpair.first),beta*tfpair.second/numdoc);
            }
        }

    }

   return new_query;

}



int main(int argc, char* argv[])
{
    //index::register_ranker<new_ranker>();
    if (argc != 2 && argc != 3)
    {
        std::cerr << "Usage:\t" << argv[0] << " configFile" << std::endl;
        return 1;
    }

    // Log to standard error
    logging::set_cerr_logging();

    // Register additional analyzers
    parser::register_analyzers();
    sequence::register_analyzers();




    // Submission-specific - Ignore
    std::ofstream submission;

    submission.open("Assignment2/output.txt");
    if (!submission.is_open())
    {
        std::cout<<"Problem writing the output to the system. Make sure the program has enough writing privileges. Quiting..."<<std::endl;
        return 0;
    }
    std::string nickname;
    std::cout<<"Enter your nickname: ";
    std::getline(std::cin,nickname);
    submission<<nickname+'\n';
    // End of the submission-specific code



    //  Create an inverted index using a DBLRU cache.
    auto idx = index::make_index<index::dblru_inverted_index>(argv[1], 30000);

    // Create a forward index for rocchio feedback
    auto fidx = meta::index::make_index<index::memory_forward_index>(argv[1]); // fidx is a pointer to the forward index

    // Create a ranking class based on the config file.
    auto config = cpptoml::parse_file(argv[1]);
    auto group = config.get_table("ranker");
    if (!group)
        throw std::runtime_error{"\"ranker\" group needed in config file!"};
    auto ranker = index::make_ranker(*group);

    // Get the path to the file containing queries
    auto query_path = config.get_as<std::string>("querypath");
    if (!query_path)
        throw std::runtime_error{"config file needs a \"querypath\" parameter"};

    std::ifstream queries{*query_path + *config.get_as<std::string>("dataset")
                          + "-queries.txt"};

    // Create an instance of ir_eval to evaluate the MAP and Precision@10 for the training queries
    auto eval = index::ir_eval(argv[1]);


    // Print the precision@10 and the MAP for the training queries
    size_t i = 1;
    std::string content;
    while (i<=100 && queries.good())
    {
        std::getline(queries, content); // Read the content of the current training query from file

        corpus::document query{"", doc_id{i-1}}; // Instantiate the query as an empty document

        query.content(content); // Set the content of the query

        std::cout << "Ranking query " << i++ << ": " << query.content() << std::endl;

        auto ranking = ranker->score(*idx, query, 50); // ranking is a vector of pairs of the form <docID,docScore>
        // You can access the ith document's ID using ranking[i].first and its score using ranking[i].second

        // with rocchio feedback
        /*
        auto ranking = ranker->score(*idx, query, 3);
        auto new_query = Rocchio(*idx, *fidx, query, ranking); // You should implement this function
        ranking = ranker->score(*idx, new_query, 50);
        */


        std::cout<< "Precision@10 for this query: "<< eval.precision(ranking,query.id(),10) << std::endl;

        eval.avg_p(ranking,query.id(),50); // Store the average precision at 50 documents for the current query

        std::cout << "Showing top 10 of " << ranking.size() << " results."<< std::endl;

        for (size_t i = 0; i < ranking.size() && i < 10; ++i) // Loop over the top 10 documents in ranking
            std::cout << (i + 1) << ". " << " " << idx->doc_path(ranking[i].first)
                      << " " << ranking[i].second << std::endl;

        std::cout << std::endl;

    }

    std::cout<< "The MAP for the training queries is: " << eval.map() << std::endl;

    // Write the top 50 documents of each test query to the submission file
    while (queries.good())
    {
        std::getline(queries, content); // Read the content of the current testing query from file

        corpus::document query;

        query.content(content);

        auto ranking = ranker->score(*idx, query, 50);

        for (size_t i=0; i < ranking.size() && i<50; i++) // Loop over the top 50 documents
        {
            submission<< std::to_string(ranking[i].first)+" "; // Write the IDs of the top 50 documents to output.txt
        }
        submission<<"\n";

    }


    submission.close();
    return 0;
}
