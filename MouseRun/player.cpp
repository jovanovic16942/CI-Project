#include "player.h"
#include "cheese.h"
#include "mousetrap.h"
#include "waterpool.h"
#include "cat.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QTimer>
#include <QDebug>

const int Player::maxSpeed = 6;
const qreal Player::turningAngle = 0.5;
const qreal Player::consumption = 0.01;

Player::Player()
    :
      angle{0},
      traveled{0},
      rotated{0},
      speed{5},
      alive{true},
      advanceBonus{0},
      inWater{false},
      color{QRandomGenerator::global()->bounded(256),
            QRandomGenerator::global()->bounded(256),
            QRandomGenerator::global()->bounded(256)
            },
      color2{QRandomGenerator::global()->bounded(256),
             QRandomGenerator::global()->bounded(256),
             QRandomGenerator::global()->bounded(256)
             }
{
    keysDown['w'] = false;
    keysDown['a'] = false;
    keysDown['s'] = false;
    keysDown['d'] = false;

    setZValue(2);

    setPos(0, 0);

//    // Update the player
    static QTimer updateTimer;
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(update()));
    updateTimer.start(15);

}

// Taken from the CollidingMice example
QRectF Player::boundingRect() const
{
    qreal adjust = 0.5;
    return QRectF(-18 - adjust, -22 - adjust,
                  36 + adjust, 60 + adjust);
}

QRectF Player::fieldOfVisionForward() const
{
    return QRectF(-20, -70, 40, 70);
}

QRectF Player::fieldOfVisionLeft() const
{
    return QRectF(-60, -50, 40, 50);
}

QRectF Player::fieldOfVisionRight() const
{
    return QRectF(20, -50, 40, 50);
}

// Taken from the CollidingMice example
QPainterPath Player::shape() const
{
    QPainterPath path;
    path.addRect(-10, -20, 20, 40);
    return path;
}

// Taken from the CollidingMice example and slightly modified
void Player::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing);


    // Body
    painter->setBrush(color);
    painter->drawEllipse(-10, -20, 20, 40);

    // Eyes
    painter->setBrush(Qt::white);
    painter->drawEllipse(-10, -17, 8, 8);
    painter->drawEllipse(2, -17, 8, 8);

    // Nose
    painter->setBrush(Qt::black);
    painter->drawEllipse(QRectF(-2, -22, 4, 4));

    // Pupils
    painter->drawEllipse(QRectF(-8.0, -17, 4, 4));
    painter->drawEllipse(QRectF(4.0, -17, 4, 4));

    // Ears
    painter->setBrush(color2);
    painter->drawEllipse(-17, -12, 16, 16);
    painter->drawEllipse(1, -12, 16, 16);

    // Tail
    QPainterPath path(QPointF(0, 20));
    path.cubicTo(-5, 22, -5, 22, 0, 25);
    path.cubicTo(5, 27, 5, 32, 0, 30);
    path.cubicTo(-5, 32, -5, 42, 0, 35);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);
}

void Player::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_A || event->key() == Qt::Key_Left){
        keysDown['a'] = true;
    }
    if(event->key() == Qt::Key_D || event->key() == Qt::Key_Right){
        keysDown['d'] = true;
    }
    if(event->key() == Qt::Key_W || event->key() == Qt::Key_Up){
        keysDown['w'] = true;
    }
    if(event->key() == Qt::Key_S || event->key() == Qt::Key_Down){
        keysDown['s'] = true;
    }
}

void Player::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_A || event->key() == Qt::Key_Left){
        keysDown['a'] = false;
    }
    if(event->key() == Qt::Key_D || event->key() == Qt::Key_Right){
        keysDown['d'] = false;
    }
    if(event->key() == Qt::Key_W || event->key() == Qt::Key_Up){
        keysDown['w'] = false;
    }
    if(event->key() == Qt::Key_S || event->key() == Qt::Key_Down){
        keysDown['s'] = false;
    }
}

double Player::calcFitness()
{
    double fitness = 0;

    if(rotated){
        fitness += advanceBonus;
    }

    if(traveled){
        fitness += advanceBonus;
    }

    fitness += traveled;
    fitness += rotated;

    return fitness >= 0 ? fitness : 0;
}


void Player::move()
{
    // MOVE
    QPointF newPos;
    // Move forward
    if(keysDown['w'] && !keysDown['s']){
        if(inWater){
            newPos = mapToParent(0, -1);
        }else{
            newPos = mapToParent(0, -speed);
            advanceBonus++;
            traveled += speed;
        }

        setPos(newPos.x(), newPos.y());
    }
    // Move backward
    else if(keysDown['s'] && !keysDown['w']){
        if(inWater){
            newPos = mapToParent(0, -1);
        }else{
            newPos = mapToParent(0, speed/2);
        }
        setPos(newPos.x(), newPos.y());
    }

    // ROTATE
    // Rotate left
    if(keysDown['a'] && !keysDown['d']){
        if(inWater){
            angle = -0.1;
        }else{
            angle = -turningAngle;
        }
    }
    // Rotate right
    else if(keysDown['d'] && !keysDown['a']){
        if(inWater){
            angle = 0.1;
        }else{
            angle = turningAngle;
        }
    }

    // Rotate
    if(angle){
        qreal dx = sin(angle) * 20;
        setRotation(rotation() + dx);
        angle = 0;
        rotated += abs(dx);
    }
}

void Player::update()
{
    inWater = false;

    // Player is losing speed over time
    if(speed > maxSpeed/2)
        speed -= consumption;


    // Check for collision with other items
    QList<QGraphicsItem*> collided = collidingItems();
    foreach(QGraphicsItem* item, collided){

        // If the item is a cheese, eat it
        if(Cheese *cheese = dynamic_cast<Cheese*>(item)){
            advanceBonus++;
            speed = maxSpeed;
        }
        // If the item is a trap, die
        else if(dynamic_cast<MouseTrap*>(item)){
            alive = false;
        }

        else if(dynamic_cast<WaterPool*>(item)){
            inWater = true;
            advanceBonus--;
        }
    }

    move();
}
