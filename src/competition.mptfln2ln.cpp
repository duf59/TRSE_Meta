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

// mptf2ln_ranker is a class that implements the mptf2ln ranking function. It is derived from the base class ranker.
class mptf2ln_ranker: public index::ranker
{
private: // s_, mu_, alpha_ and lambda_ are the parameters of mptf2ln
    double s_ = 0.2;
    double mu_ = 2000;
    double alpha_ = 0.3;
    double lambda_ = 0.7;

public:
    const static std::string id;
    mptf2ln_ranker(); // Default Constructor
    mptf2ln_ranker(double s, double mu, double alpha, double lambda); // Constructor which can set parameter values
    void set_param(double s, double mu, double alpha, double lambda){s_ = s; mu_ = mu; alpha_ = alpha; lambda_ = lambda;}; // Setter
    double score_one(const index::score_data&); // Calculates the score for a single matched term
};

const std::string mptf2ln_ranker::id = "mptf2ln"; // Used to identify mptf2ln_ranker in config.toml
mptf2ln_ranker::mptf2ln_ranker(){}
mptf2ln_ranker::mptf2ln_ranker(double s, double mu, double alpha, double lambda) : s_{s}, mu_{mu}, alpha_{alpha}, lambda_{lambda} {}

double mptf2ln_ranker::score_one(const index::score_data& sd)
{
    /*
    This function is called for each matched term between the query and the document.
    The function's argument is a struct that contains important information about
    the matched term. For example, sd.doc_term_count gives the # of occurrences of
    the term in the document.
    */
    double doc_len = sd.idx.doc_size(sd.d_id); // Current document length
    double avg_dl = sd.avg_dl; // Average document length in the corpus
    double tf = sd.doc_term_count; // Raw term count in the document
    double df = sd.doc_count; // number of docs that term appears in
    double pc = static_cast<double>(sd.corpus_term_count) / sd.total_terms;


    double s = s_; // mptf2ln's parameter
    double mu = mu_; // mptf2ln's parameter
    double alpha = alpha_; // mptf2ln's parameter
    double lambda = lambda_; // mptf2ln's parameter

    double tfok = 2.2*tf/(1.2+tf); // okapi tf term
    double idfpiv = std::log((sd.num_docs + 1.0)/df);
    double tfidfdir = std::log(1.0 + tf/(mu*pc));
    double lnpiv = 1 - s + s*doc_len/avg_dl;

    double tfidf2 = alpha*tfok*idfpiv + (1.0 - alpha)*tfidfdir;

    double score = sd.query_term_count*tfidf2/std::pow(lnpiv,lambda);

    return score; // Change 0 to the final score you calculated
}



void mptf2ln_tune (const std::shared_ptr<index::dblru_inverted_index> & idx, std::vector<corpus::document> & allqueries, index::ir_eval & eval, double & alpha, double & lambda, double & s, double & mu, int & Q)
{

    double alphavalues [10] = {0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.6}; // Different values for the parameter alpha
    double lambdavalues [9] = {0.5, 0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9}; // Different values for the parameter lambda
    double svalues [9] = {0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6}; // Different values for the parameter alpha
    double muvalues [1] = {2000.0}; // Different values for the parameter lambda
    unsigned int Qvalues [5] = {10, 20, 30, 50, 100};
    double maxmap = 0; // Stores the current maximum MAP value
    double smax = 0.2;
    double mumax = 2000;
    double alphamax = 0.3; // Stores the current optimal alpha (i.e. c that achieves max MAP) - Ignore the initial value
    double lambdamax = 0.7; // Stores the current optimal lambda - Ignore the initial value
    int Qmax = 50;
    double progress = 0.0;

    std::ofstream writeout;

    writeout.open("Assignment2/tuning.txt");
    if (!writeout.is_open())
    {
        std::cout<<"Problem writing the output of tuning to the system. Make sure the program has enough writing privileges. Quiting..."<<std::endl;
    }
    writeout <<  "s" << "\t" << "mu" << "\t" << "alpha" << "\t" << "lambda" << "\t" << "Q" << "\t" << "MAP" << "\n";

    auto ranker = make_unique<mptf2ln_ranker>(); // creates a pointer to a mptf2ln_ranker instance

    for (int i=0 ; i<10 ; i++) // Loops over all alpha values
    {
        for (int j=0 ; j<9 ; j++) // Loops over all lambda values
         {
            for (int k=0 ; k<9 ; k++)
                {
                    for (int l=0 ; l<1; l++)
                    {
                        for (int m=0 ; m<5 ; m++)
                            {
                                ranker->set_param(svalues[k], muvalues[l], alphavalues[i],lambdavalues[j]); // Sets the parameters of ranker to the current values of alpha and lambda

                                progress = progress + 1.0;
                                std::cout<<"Progress " << std::setprecision(3) << progress*100.0/(10.0*9.0*9.0*5.0) << std::endl;


                                for (std::vector<corpus::document>::iterator query = allqueries.begin(); query != allqueries.end(); ++query) // Iterates over all queries in allqueries
                                {
                                    auto ranking = ranker->score(*idx, *query, Qvalues[m]); // Returns a ranked list of the top 1000 documents for the current query
                                    eval.avg_p(ranking,(*query).id(),Qvalues[m]); // eval.avg_p stores the value of average precision for the current query in the instance eval
                                }
                                if (eval.map() > maxmap) // Updates maxmap, alphamax, lambdamax if the current map, which is equal to eval.map(), is greater than maxmap
                                {
                                    // You should only change the values of the following three assignments
                                    maxmap = eval.map(); // Change 0 to the correct value DONE
                                    alphamax = alphavalues[i]; // Change 0 to the correct value DONE
                                    lambdamax = lambdavalues[j]; // Change 0 to the correct value DONE
                                    smax = svalues[k];
                                    mumax = muvalues[l];
                                    Qmax = Qvalues[m];
                                }


                                writeout <<  svalues[k] << "\t" << muvalues[l] << "\t" << alphavalues[i] << "\t" << lambdavalues[j] << "\t" << Qvalues[m] << "\t" << eval.map() << "\n";

                                eval.reset_stats(); // Deletes all the average precision values stored in eval to allow correct calculation of MAP for the next parameter combination

                            }
                    }
                }
         }
    }
    std::cout<<"Max MAP = "<< maxmap << " achieved by " << "s = " << smax << ", mu = " << mumax << ", alpha = " << alphamax << ", lambda = " << lambdamax << std::endl;
    alpha = alphamax; // Returns the best c value to the calling function
    lambda = lambdamax; // Returns the best lambda value to the calling function
    s = smax;
    mu = mumax;
    Q = Qmax;
}



namespace meta{
namespace index{
template <>
std::unique_ptr<ranker>make_ranker<mptf2ln_ranker>(
        const cpptoml::table & config) // Used by mptf2ln_ranker to read the parameters c and lambda from config.toml - You can ignore it
{
    double s = 0.2;
    if (auto s_file = config.get_as<double>("s"))
        s = *s_file;

    double mu = 2000;
    if (auto mu_file = config.get_as<double>("mu"))
        mu = *mu_file;

    double alpha = 0.3;
    if (auto alpha_file = config.get_as<double>("alpha"))
        alpha = *alpha_file;

    double lambda = 0.7;
    if (auto lambda_file = config.get_as<double>("lambda"))
        lambda = *lambda_file;

    return make_unique<mptf2ln_ranker>(s, mu, alpha, lambda);
}

}
}



int main(int argc, char* argv[])
{
    //index::register_ranker<new_ranker>();
    index::register_ranker<mptf2ln_ranker>(); // Registers mptf2ln_ranker so that you can use it in config.toml



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
    double alpha = 0.3;
    double lambda = 0.7;
    double s = 0.2;
    double mu = 2000;
    int Q = 50;
    mptf2ln_tune (idx, trainqueries, eval, alpha, lambda, s, mu, Q);
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
