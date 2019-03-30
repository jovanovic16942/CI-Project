#include "controller.h"
#include "game.h"
#include <cmath>
#include <QTime>

const int populationSize = 50;
const int numOfGenerations = 20;
// number of inputs and outputs for the genome (nn)
const int numInputs = 4;
const int numOutputs = 2;

Controller::Controller()
    : generationNum{0},
      nextConnId{0},
      nextNodeId{numInputs + numOutputs + 1}
{
    // create initial population
    for(int i = 0; i < populationSize; i++) {

        Genome *genome = new Genome(numInputs, numOutputs);
        population.push_back(*genome);

        connect(genome, SIGNAL(nodeIdNeeded(Genome*, int)),
                this, SLOT(getNodeId(Genome*, int)),
                Qt::DirectConnection);

        connect(genome, SIGNAL(connectionIdNeeded(Genome*, int, int)),
                this, SLOT(getConnId(Genome*, int, int)),
                Qt::DirectConnection);
    }

    for(int i = 0; i < numOfGenerations; i++) {
        evolve();
    }
}


void Controller::getNodeId(Genome *genome, int connectionId)
{
    auto search = mapNode.find(connectionId);
    if(search == mapNode.end()) {
        mapNode[connectionId] = nextNodeId++;
    }
    genome->newNodeId = mapNode[connectionId];
}

void Controller::getConnId(Genome *genome, int fromNodeId, int toNodeId)
{
    auto key = std::make_pair(fromNodeId, toNodeId);
    auto search = mapConn.find(key);
    if(search == mapConn.end()) {
        mapConn[key] = nextConnId++;
    }
    genome->newConnectionId = mapConn[key];
}

void Controller::evolve()
{
    // Calculate fitness as the time a player stays alive
    for(auto &genome : population) {

        QTime time;
        time.start();
        Game game(&genome);
        genome.fitness = time.elapsed();
    }

    // Speciate
    for(auto & genome : population){
        bool newSpecies = true;
        for(auto & s : species) {
            if(s.isSameSpecies(genome)) {
                s.addToSpecies(&genome);
                newSpecies = false;
                break;
            }
        }

        if (newSpecies) {
            species.push_back(Species(&genome));
        }
    }

    // Create new population
    std::vector<Genome> newPopulation;

    double averageFitnessSum = 0;

    for(size_t i = 0; i < species.size(); i++) {

        species[i].sortGenomesByFitness();
        species[i].decimateSpecies();
        species[i].calcAverageFitness();
        species[i].explicitFitnessSharing();
        averageFitnessSum += species[i].averageFitness;
    }

    std::sort(species.begin(), species.end(),
              [](const Species &a, const Species &b){return a.averageFitness > b.averageFitness;} );

    for(size_t i = 0; i < species.size(); i++) {

        if (!species[i].allowedReproduction) {
            continue;
            // maybe delete this species
        }

        newPopulation.push_back(*species[i].representGenome->clone());

        int numOffspring = std::floor(species[i].averageFitness / averageFitnessSum * populationSize) - 1;

        for(int i = 0; i < numOffspring; i++) {
            newPopulation.push_back(*species[i].createOffspring());
        }
    }

    while(newPopulation.size() < populationSize) {

        newPopulation.push_back(*species[0].createOffspring());
    }

    // TODO interspecies breeding (0.001 prob)

    population.clear();
    for(size_t i = 0; i < populationSize; i++) {
        population.push_back(newPopulation[i]);
    }
}