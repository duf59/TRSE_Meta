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

// bm25_ranker is a class that implements the bm25 ranking function. It is derived from the base class ranker.
class bm25_ranker: public index::ranker
{
private: // c_ and lambda_ are the parameters of bm25
    double k1_ = 1.2;
    double b_ = 0.75;
    double k3_ = 500.0;

public:
    const static std::string id;
    bm25_ranker(); // Default Constructor
    bm25_ranker(double k1, double b, double k3); // Constructor which can set parameter values
    void set_param(double k1, double b, double k3){k1_ = k1; b_= b; k3_ = k3;}; // Setter
    double score_one(const index::score_data&); // Calculates the score for a single matched term
};

const std::string bm25_ranker::id = "mybm25"; // Used to identify bm25_ranker in config.toml
bm25_ranker::bm25_ranker(){}
bm25_ranker::bm25_ranker(double k1, double b, double k3) : k1_{k1}, b_{b}, k3_{k3} {}

double bm25_ranker::score_one(const index::score_data& sd)
{
    double doc_len = sd.idx.doc_size(sd.d_id);

    // add 1.0 to the IDF to ensure that the result is positive
    double IDF = std::log(
        1.0 + (sd.num_docs - sd.doc_count + 0.5) / (sd.doc_count + 0.5));

    double TF = ((k1_ + 1.0) * sd.doc_term_count)
                / ((k1_ * ((1.0 - b_) + b_ * doc_len / sd.avg_dl))
                   + sd.doc_term_count);

    double QTF = ((k3_ + 1.0) * sd.query_term_count)
                 / (k3_ + sd.query_term_count);

    return TF * IDF * QTF;
}



void bm25_tune (const std::shared_ptr<index::dblru_inverted_index> & idx, std::vector<corpus::document> & allqueries, index::ir_eval & eval, double & k1, double & b, double & k3)
{

    double k1values [12] = {1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0, 3.0, 4.0, 5.0}; // Different values for the parameter k1
    double bvalues [12] = {0.60, 0.65, 0.70, 0.75, 0.80, 0.85, 0.90, 0.95, 1.0, 1.5, 2.0, 3.0}; // Different values for the parameter b
    double k3values [7] = {200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0};// Different values for parameter k3
    double maxmap = 0; // Stores the current maximum MAP value
    double k1max = 1.2;
    double bmax = 0.75;
    double k3max = 500.0;
    std::ofstream writeout;

    writeout.open("Assignment2/tuning.txt");
    if (!writeout.is_open())
    {
        std::cout<<"Problem writing the output of tuning to the system. Make sure the program has enough writing privileges. Quiting..."<<std::endl;
    }
    writeout <<  "k1" << "\t" << "b" << "\t" << "k3" << "\t" << "MAP" << "\n";

    auto ranker = make_unique<bm25_ranker>(); // creates a pointer to a bm25_ranker instance

    for (int i=0 ; i<12 ; i++) // Loops over all k1 values
    {
        for (int j=0 ; j<12 ; j++) // Loops over all b values
         {
            for (int k=0 ; k<7 ; k++) // Loops over all k3 values
            {
                ranker->set_param(k1values[i],bvalues[j],k3values[k]); // Sets the parameters of ranker to the current values
                std::cout << "Tuning for parameters : " << "k1 = " << k1values[i] << "; b = " << bvalues[j] << "; k3 = " << k3values[k] << std::endl;
                for (std::vector<corpus::document>::iterator query = allqueries.begin(); query != allqueries.end(); ++query) // Iterates over all queries in allqueries
                {
                    auto ranking = ranker->score(*idx, *query, 50); // Returns a ranked list of the top 1000 documents for the current query
                    eval.avg_p(ranking,(*query).id(),50); // eval.avg_p stores the value of average precision for the current query in the instance eval
                }

                writeout <<  k1values[i] << "\t" << bvalues[j] << "\t" << k3values[k] << "\t" << eval.map() << "\n";


                if (eval.map() > maxmap) // Updates maxmap, cmax, lambdamax if the current map, which is equal to eval.map(), is greater than maxmap
                {
                    // You should only change the values of the following three assignments
                    maxmap = eval.map(); // Change 0 to the correct value DONE
                    k1max = k1values[i]; // Change 0 to the correct value DONE
                    bmax = bvalues[j]; // Change 0 to the correct value DONE
                    k3max = k3values[k];
                }

                eval.reset_stats(); // Deletes all the average precision values stored in eval to allow correct calculation of MAP for the next parameter combination
            }
         }
    }

    writeout.close();
    std::cout<<"Max MAP = "<< maxmap << " achieved by " << "k1 = " << k1max << ", b = " << bmax << ", k3 = " << k3max << std::endl; // Prints to the standard ouput
    k1 = k1max; // Returns the best c value to the calling function
    b = bmax;
    k3 = k3max; // Returns the best lambda value to the calling function
}

namespace meta{
namespace index{
template <>
std::unique_ptr<ranker>make_ranker<bm25_ranker>(
        const cpptoml::table & config) // Used by bm25_ranker to read the parameters c and lambda from config.toml - You can ignore it
{
    double k1 = 1.2;
    if (auto k1_file = config.get_as<double>("k1"))
        k1 = *k1_file;

    double b = 0.75;
    if (auto b_file = config.get_as<double>("b"))
        b = *b_file;

    double k3 = 500.0;
    if (auto k3_file = config.get_as<double>("k3"))
        k3 = *k3_file;

    return make_unique<bm25_ranker>(k1, b, k3);
}

}
}



int main(int argc, char* argv[])
{
    //index::register_ranker<new_ranker>();
    index::register_ranker<bm25_ranker>(); // Registers bm25_ranker so that you can use it in config.toml



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

    std::vector<corpus::document> trainqueries; // will contain train quesries (used for tuning function)

    while (i<=100 && queries.good())
    {
        std::getline(queries, content); // Read the content of the current training query from file

        corpus::document query{"", doc_id{i-1}}; // Instantiate the query as an empty document

        query.content(content); // Set the content of the query

        trainqueries.push_back(query);

        std::cout << "Ranking query " << i++ << ": " << content<< std::endl;

        auto ranking = ranker->score(*idx, query, 50); // ranking is a vector of pairs of the form <docID,docScore>
        // You can access the ith document's ID using ranking[i].first and its score using ranking[i].second

        std::cout<< "Precision@10 for this query: "<< eval.precision(ranking,query.id(),10) << std::endl;

        eval.avg_p(ranking,query.id(),50); // Store the average precision at 50 documents for the current query

        std::cout << "Showing top 10 of " << ranking.size() << " results."<< std::endl;

        for (size_t i = 0; i < ranking.size() && i < 10; ++i) // Loop over the top 10 documents in ranking
            std::cout << (i + 1) << ". " << " " << idx->doc_path(ranking[i].first)
                      << " " << ranking[i].second << std::endl;

        std::cout << std::endl;

    }

    std::cout<< "The MAP for the training queries is: " << eval.map() << std::endl;

    // tuning
    /*
    double k1 = 1.2;
    double b = 0.75;
    double k3 = 500.0;
    bm25_tune (idx, trainqueries, eval, k1, b, k3);
    */

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
