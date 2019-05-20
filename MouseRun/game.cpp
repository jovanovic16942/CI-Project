#include "game.h"

#include "cheese.h"
#include "mousetrap.h"
#include "waterpool.h"
#include <QTimer>
#include <QDebug>
#include <QRandomGenerator>

const int areaH = 400;

Game::Game(std::vector<Genome*> genomes, unsigned bId)
    : bestI{0},
      bId{bId},
      genomes{genomes}
{
    numOfAlive = genomes.size();


    // initialize players
    for(size_t i = 0; i < genomes.size(); i++){
        Player* player = new Player();
        mice.push_back(player);
    }

    // Create the scene
    int width = 600;
    int height = 800;

    scene = new QGraphicsScene(this);
    setAlignment(Qt::AlignCenter);

    setFixedSize(width, height);
    scene->setSceneRect(-300, -300, width, height);
    setScene(scene);

    // Turn off the scrollbars
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setBackgroundBrush(QBrush(QColor(55,205,55)));


    // Window for nn
    nnView = new QGraphicsView();
    nnView->setScene(new QGraphicsScene(nnView));
//    nnView->setSceneRect(0, 0, 300, 800);
    nnView->setRenderHint(QPainter::Antialiasing);
    // Start the game
    start();
}

void Game::start()
{
    // Spawn players
    for(size_t i = 0; i < mice.size(); i++){
        mice[i]->setPos(QRandomGenerator::global()->bounded(-200,200)
                        , -300);
        scene->addItem(mice[i]);
    }

    // Spawn cat
    cat = new Cat();
    cat->setPos(0, 0);
    scene->addItem(cat);

    // Update the Game
    static QTimer updateTimer;
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(update()));
    updateTimer.start(15);

    // Setup left and right bound
    boundW = 500;
    leftBound  = new WaterBound(15000, boundW);
    rightBound = new WaterBound(15000, boundW);

    leftBound->setPos(-boundW, 0);
    rightBound->setPos(boundW, 0);

    scene->addItem(leftBound);
    scene->addItem(rightBound);


    // initialize item spawning
//    spawnObjectsInArea(1);
    spawnObjectsInArea(2);
    spawnObjectsInArea(3);
    spawnObjectsInArea(4);
    bla = areaH;
    areaNum = 4;

    // Show the scene
    drawGenome(genomes[0]);
    show();
    nnView->show();
}

void Game::makeDecisions()
{

    // make mice think
    for(size_t i = 0; i < mice.size(); i++){
        if(mice[i] != nullptr && mice[i]->alive){
            // feed forward...
            std::vector<double> inputs;
            inputs.push_back(mice[i]->pos().x());
            inputs.push_back(mice[i]->pos().y());
            inputs.push_back(mice[i]->speed);
            inputs.push_back(mice[i]->rotation());

//            qDebug() << scene->items().size();
            // Mouse can only see items 1 area ahead (2 x (2 traps, 2 pools, 8 cheese) = 24 items)
            std::vector<QGraphicsItem*> currentArea;
            std::vector<QGraphicsItem*> nextArea;

            foreach (QGraphicsItem* item, scene->items()) {

                if(WaterBound* x = dynamic_cast<WaterBound*>(item)) { continue; }
                else if(Cheese* x = dynamic_cast<Cheese*>(item)) {}
                else if(MouseTrap* x = dynamic_cast<MouseTrap*>(item)) {}
                else if(WaterPool* x = dynamic_cast<WaterPool*>(item)) {}
                else {
                    continue;
                }

                double mousePos = mice[i]->pos().y();
                double itemPos = item->pos().y();
                double firstAreaStart = int(mousePos / areaH) * areaH + (areaH - bla);
                double nextAreaStart = firstAreaStart - areaH;
                double nextAreaEnd = nextAreaStart - areaH;


                if(itemPos < firstAreaStart && itemPos > nextAreaStart){
                    currentArea.push_back(item);
                }else if(itemPos < nextAreaStart && itemPos > nextAreaEnd){
                    nextArea.push_back(item);
                }
            }

            if(currentArea.empty()){
                for(size_t j=0; j<24/*magic*/; j++){
                    inputs.push_back(0.0);
                }
            }

//            qDebug() << currentArea.size() << nextArea.size();

            for(size_t j=0; j<currentArea.size(); j++){
                if(Cheese* cheese = dynamic_cast<Cheese*>(currentArea[j])){
                    inputs.push_back(cheese->pos().x());
                    inputs.push_back(cheese->pos().y());
//                    inputs.push_back(100);
//                    qDebug() << "1 cheese: " << j;
                }
            }
            for(size_t j=0; j<currentArea.size(); j++){
                if(WaterPool* pool = dynamic_cast<WaterPool*>(currentArea[j])){
                    inputs.push_back(pool->pos().x());
                    inputs.push_back(pool->pos().y());
//                    inputs.push_back(-50);
//                    qDebug() << "1 pool: " << j;
                }
            }
            for(size_t j=0; j<currentArea.size(); j++){
                if(MouseTrap* trap = dynamic_cast<MouseTrap*>(currentArea[j])){
                    inputs.push_back(trap->pos().x());
                    inputs.push_back(trap->pos().y());
//                    inputs.push_back(-500);
//                    qDebug() << "1 trap: " << j;
                }
            }

            for(size_t j=0; j<nextArea.size(); j++){
                if(Cheese* cheese = dynamic_cast<Cheese*>(nextArea[j])){
                    inputs.push_back(cheese->pos().x());
                    inputs.push_back(cheese->pos().y());
//                    inputs.push_back(100);
//                    qDebug() << "2 cheese: " << j;
                }
            }
            for(size_t j=0; j<nextArea.size(); j++){
                if(WaterPool* pool = dynamic_cast<WaterPool*>(nextArea[j])){
                    inputs.push_back(pool->pos().x());
                    inputs.push_back(pool->pos().y());
//                    inputs.push_back(-50);
//                    qDebug() << "2 pool: " << j;
                }
            }
            for(size_t j=0; j<nextArea.size(); j++){
                if(MouseTrap* trap = dynamic_cast<MouseTrap*>(nextArea[j])){
                    inputs.push_back(trap->pos().x());
                    inputs.push_back(trap->pos().y());
//                    inputs.push_back(-500);
//                    qDebug() << "2 trap: " << j;
                }
            }

//            qDebug() << inputs.size();


            std::vector<double> outputs = genomes[i]->feedForward(inputs);
            mice[i]->keysDown['w'] = outputs[0] >= 0.5;
            mice[i]->keysDown['a'] = outputs[1] >= 0.5;
            mice[i]->keysDown['s'] = outputs[2] >= 0.5;
            mice[i]->keysDown['d'] = outputs[3] >= 0.5;
        }
    }
}

void Game::update()
{

    //    qDebug() << bestI;

    // check for dead mice, determine best mouse
    for(size_t i = 0; i < mice.size(); i++){
        if(!mice[i]){
            continue;
        }
        else if (!mice[i]->alive){
            // A player died
//            qDebug() << "time" << time.elapsed();

            // fitness function
            double fitness = mice[i]->calcFitness();
            emit died(bId + i, fitness);
            delete mice[i];
            mice[i] = nullptr;
            numOfAlive--;
        }else{
            if(!mice[bestI] || mice[i]->pos().y() < mice[bestI]->pos().y()){
                bestI = i;
                drawGenome(genomes[bestI]);
            }
        }
    }

    if(numOfAlive == 0){
        delete nnView;
        deleteLater();
        return;
    }

    if(!mice[bestI]){
        for(size_t i = 0; i < mice.size(); i++){
            if(mice[i]){
                bestI = i;
                break;
            }
        }
    }

    // Move the cat == move everything else
    foreach (QGraphicsItem* item, scene->items()) {
        if(Cat *cat = dynamic_cast<Cat*>(item)) continue;
        item->setPos(item->pos().x(), item->pos().y() + cat->speed);
    }

    // Water bound is moving with player
    // If you don't know why 200, enter scene height (=600) or similar, see what happens
    if (leftBound->pos().y() > 200) {
        leftBound->setPos(leftBound->pos().x(), 0);
        rightBound->setPos(rightBound->pos().x(), 0);
    }
    // If mouse mouse goes backward, move water bound below
    if (leftBound->pos().y() < -200) {
        leftBound->setPos(leftBound->pos().x(), 0);
        rightBound->setPos(rightBound->pos().x(), 0);
    }

    // run game normaly
    focusBest();
    spawnObjects();
    deleteObjects();
    makeDecisions();
}

void Game::focusBest(){

    setSceneRect(mice[bestI]->sceneBoundingRect());
}

void Game::drawGenome(Genome *gen)
{

    nnView->scene()->clear();

    double d = 10;

    double offset = 2*d;
    double offsetL = 5*d;

    std::map<size_t, size_t> layers;

    for (size_t i = 0; i < gen->nodes.size(); i++){
        QGraphicsEllipseItem* node = new QGraphicsEllipseItem(0, 0, d, d);
        size_t l = gen->nodes[i]->layer;
        if(layers.count(l) == 0){
            layers[l] = 1;
        }else{
            layers[l]++;
        }
//        qDebug() << l;
        node->setPos(l * offsetL, layers[l] * offset);
        nnView->scene()->addItem(node);
    }

    for(size_t i = 0; i < gen->connections.size(); i++){


        size_t n1Layer = gen->connections[i]->inNode->layer;
        size_t n2Layer = gen->connections[i]->outNode->layer;

        double n1x = n1Layer * offsetL;
        double n1y = layers[n1Layer] * offset;
        double n2x = n2Layer * offsetL;
        double n2y = layers[n2Layer] * offset;

        int r = 0;
        int g = 0;
        int b = 0;


        if(gen->connections[i]->enabled){

            double w = gen->connections[i]->weight;
            if(w > 0){
                g = w * 255;
            }else{
                r = -w * 255;
            }
        }else{
            b = 255;
        }

        QColor color(r,g,b);
        nnView->scene()->addLine(n1x + d/2, n1y + d/2, n2x + d/2, n2y + d/2, QPen(color));
    }

    layers.clear();
    nnView->scene()->setSceneRect(nnView->scene()->itemsBoundingRect());
    nnView->fitInView(nnView->scene()->sceneRect(), Qt::KeepAspectRatio);
}

void Game::spawnObjectsInArea(int area)
{
    //spawn 2 mousetraps, 2 waterpools and 8 pieces of cheese
    for(int i = 0; i < 12; i++) {

        QGraphicsItem *item;

        if (i < 2){
            item = new MouseTrap();

        } else if (i < 10){
            item = new Cheese();

        } else{
            item = new WaterPool(QRandomGenerator::global()->bounded(30, 100),
                                 QRandomGenerator::global()->bounded(30, 130));
        }

        item->setRotation(QRandomGenerator::global()->bounded(-180, 180));
        item->setPos(QRandomGenerator::global()->bounded(int(leftBound->pos().x() + boundW/2),
                                                         int(rightBound->pos().x() - boundW/2)),
                     - area * areaH + QRandomGenerator::global()->bounded(5, areaH - 5));

        scene->addItem(item);
    }

    QGraphicsLineItem* line = new QGraphicsLineItem(leftBound->pos().x() + boundW/2,
                                                    - area * areaH,
                                                    rightBound->pos().x() - boundW/2,
                                                    - area * areaH);
    scene->addItem(line);
}

void Game::spawnObjects()
{

    bla -= cat->speed;


    double topY = 0;
    for(size_t i=0; i<mice.size(); i++){
        if(mice[i] && mice[i]->alive && mice[i]->pos().y() < topY){
            topY = mice[i]->pos().y();
        }
    }

    if(bla <= 0){
        if(topY < (areaNum - 3) * (-areaH)){
            spawnObjectsInArea(areaNum++);
        }

        spawnObjectsInArea(areaNum);
        bla = areaH;
    }
}

void Game::deleteObjects()
{
    foreach (QGraphicsItem* item, scene->items()) {
        if (Player *player = dynamic_cast<Player*>(item)){
            if(player->pos().y() > cat->pos().y()){
                player->alive = false;
                continue;
            }
        }
        if (item->pos().y() > cat->pos().y() + 500){
            if (Cheese *cheese = dynamic_cast<Cheese*>(item)){
                cheese->deleteLater();
            }
            else if (MouseTrap *trap = dynamic_cast<MouseTrap*>(item)){
                trap->deleteLater();
            }
            else if (WaterPool *pool = dynamic_cast<WaterPool*>(item)){
                pool->deleteLater();
            }
        }
    }
}
