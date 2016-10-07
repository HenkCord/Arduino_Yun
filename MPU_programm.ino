
#include <Shieldbot.h>
#include <ArduinoJson.h>
#include <Process.h>

/* [Begin Settings] */

/* constants */
const int POINT_COUNT = 4; //количество точек
const int POINT_START = 1; //точка начала
const int POINT_FINISH = 1; //точка конца, т.е. вернуться на базу
const String URL_API = "http://192.168.99.5:9002/rtlscp/location/B0B448D4E382/get";//ссылка на API
//const float pi = 3.14;

// Состояние скорости правого и левого колес, задаваемых в init Move
//ВАЖНО. 
//ехать прямо drive(127,127) 
//разворот на месте налево drive(-128,127) 
//разворот на месте направо drive(127,-128) 
//ехать назад drive(-128,-128) 

//MOVELEFT
const int SPEED_LEFT_WHEEL_ON_MOVELEFT = -32; 
const int SPEED_RIGHT_WHEEL_ON_MOVELEFT = 64; 
//MOVERIGHT
const int SPEED_LEFT_WHEEL_ON_MOVERIGHT = 64; 
const int SPEED_RIGHT_WHEEL_ON_MOVERIGHT = -32;
//DELAY - Время, за которое происходит поворот на определенный градус для заданной скорости
const int DELAY_15 = 100;
const int DELAY_30 = 200;
const int DELAY_45 = 300;
const int DELAY_90 = 600;
const int DELAY_120 = 800;
const int DELAY_180 = 1200;

//INPLACE - Разворот на одном месте
//MOVELEFT_INPLACE
const int SPEED_LEFT_WHEEL_ON_MOVELEFT_INPLACE = -64; 
const int SPEED_RIGHT_WHEEL_ON_MOVELEFT_INPLACE = 64; 
//MOVERIGHT_INPLACE
const int SPEED_LEFT_WHEEL_ON_MOVERIGHT_INPLACE = 64; 
const int SPEED_RIGHT_WHEEL_ON_MOVERIGHT_INPLACE = -64;
//DELAY_INPLACE - Время, за которое происходит поворот на определенный градус для заданной скорости
const int DELAY_INPLACE_90 = 500;
const int DELAY_INPLACE_120 = 600;
const int DELAY_INPLACE_180 = 800;
const int DELAY_INPLACE_360 = 1600;

//MOVEFORWARD and MOVEBACK
const int SPEED_WHEELS_ON_MOVEFORWARD = 80; 
const int SPEED_WHEELS_ON_MOVEBACK = -20; 

//Время, которое уделяется проезду вперед для определения направления движения
const int DELAY_FORWARD = 2000;

/* variables */
int point_now = 1; // текущая точка
// previous_move Хранение информации о предыдущем ходу
// complete_marker Флаг перехода к новому маркеру
bool previous_move = true, complete_marker = false; 

double x[4], y[4], radius[4];  // Необходимые координаты

double xNow, yNow, xPrevious, yPrevious, xNowDelta, yNowDelta, xPreviousDelta, yPreviousDelta;  // Текущие координаты платформы и предыдущие координаты для получения направления движения, а так же дистанции (Delta) отдаления точек от метки



//Подключение переменной для shieldbot
Shieldbot shieldbot = Shieldbot();

//Глобальная структура точек
struct Points{
  public:
  double x;
  double y;
};

//Задаёт массив координат
void setCoordinates() {

  /* [Comment] */ 
  Serial.println("Set Coordinates");
  /* [Comment] */

// БАЗА Точка 1
  x[0] = 3824246.7177767074; 
  y[0] = 8808789.96222706;
  radius[0] = 0.15; 
// Точка 2
  x[1] = 3824232.703596352; 
  y[1] = 8808788.206240144; 
  radius[1] = 0.15; 
// Точка 3
  x[2] = 3824239.0819212543; 
  y[2] = 8808785.96355696; 
  radius[2] = 0.15; 
// Точка 4
  x[3] = 3824242.7505629202; 
  y[3] = 8808783.85121144; 
  radius[3] = 0.15; 
// Точка 5
 // x[4] = 3824234.288262866; 
 // y[4] = 8808783.607217455;
 // radius[4] = 0.5; 
}

/* [Begin Settings] */

/* [Begin Utils] */

/* 
Проверка принадлежности области к текущей точке
return bool (true || false)
 */
boolean belongsArea(double x0, double y0, double rArea, double x1, double y1){

  /* [Comment] */ 
  Serial.println("check belongs Area");
  /* [Comment] */

  double h;
  h = sqrt((x0-x1)*(x0-x1)+(y0-y1)*(y0-y1));
  if (h > rArea) 
    return false;
  return true;
}

/* 
Проверка стороны наилучшего разворота.
Проверяет в какую сторону (направо или налево) нужно развернуться на 90 градусов для достижения наилучшего эффекта поворота.
!ИСПОЛЬЗОВАТЬ ТОЛЬКО ЕСЛИ МАШИНКА УДАЛЯЕТСЯ ОТ МЕТКИ ПО осям X и Y!
true - поворот направо
false - поворот налево
return bool (true || false)
x1, y1 - координаты Маркера
x2, y2 - координаты Предыдущей точки
x3, y3 - координаты Тукущей точки
 */
boolean reversalDefinition(double x1, double y1, double x2, double y2, double x3, double y3){

  /* [Comment] */ 
  Serial.println("check reversal definition");
  /* [Comment] */

  double _a, _r;
  // по формуле уравнения прямой y=ax+b 
  _a = (y2 - y1) / (x2 / x1);
  _r = y3 - (_a * x3);
  if (_r >= 0) 
    return false;
  return true;
}

/* [End Utils] */

/* [Begin API] */

//Получение координат из JSON строки
Points _deserializeJSON(String json){

  /* [Comment] */ 
  Serial.println(json);
  /* [Comment] */

  StaticJsonBuffer<900> jsonBuffer;
  JsonObject& coord = jsonBuffer.parseObject(json);

  Points result;

  if (!coord.success())
  {
    
    /* [Comment] */ 
    Serial.println("failed");
    /* [Comment] */

    //Повторное подключение через определенное количество времени
    delay(1500);

    /* [Comment] */ 
    Serial.println("new connection to server");
    /* [Comment] */

    return _deserializeJSON(json);
  }

  double x = coord["response"][0]["coord"]["loc"]["x"];
  double y = coord["response"][0]["coord"]["loc"]["y"];

  result.x = x;
  result.y = y;
  return result;
}

/*
 Получение координат
  use:
  Points coord = getPosition();
*/
Points getPosition(){

  /* [Comment] */ 
  Serial.println("get Position");
  /* [Comment] */

  Process p;        // Create a process and call it "p"
  p.begin("curl");  // Process that launch the "curl" command
  p.addParameter(URL_API); // Add the URL parameter to "curl"
  p.run();      // Run the process and wait for its termination
  String json = "";
  char c;
  while (p.available()>0) {
   c = p.read();
   json += c;
  }
  return _deserializeJSON(json);
}

/* [End API] */

/* [Begin init Move] */

/*
  Движение машинки налево по времени
*/
void moveLeft(int time){
    shieldbot.drive(SPEED_LEFT_WHEEL_ON_MOVELEFT, SPEED_RIGHT_WHEEL_ON_MOVELEFT);
    delay(time);  // Время движения в направлении
}
/*
  Движение машинки налево по времени стоя на одном месте
*/
void moveLeftinPlace(int time){
    shieldbot.drive(SPEED_LEFT_WHEEL_ON_MOVELEFT_INPLACE, SPEED_RIGHT_WHEEL_ON_MOVELEFT_INPLACE);
    delay(time);  // Время движения в направлении
}
/*
  Движение машинки направо по времени
*/
void moveRight(int time){
    shieldbot.drive(SPEED_LEFT_WHEEL_ON_MOVERIGHT, SPEED_RIGHT_WHEEL_ON_MOVERIGHT);
    delay(time);  // Время движения в направлении
}
/*
  Движение машинки направо по времени стоя на одном месте
*/
void moveRightinPlace(int time){
    shieldbot.drive(SPEED_LEFT_WHEEL_ON_MOVERIGHT_INPLACE, SPEED_RIGHT_WHEEL_ON_MOVERIGHT_INPLACE);
    delay(time);  // Время движения в направлении
}
/*
  Движение машинки прямо
*/
void moveForward(int time){
    shieldbot.drive(SPEED_WHEELS_ON_MOVEFORWARD, SPEED_WHEELS_ON_MOVEFORWARD);
    delay(time);  // Время движения в направлении
}
/*
  Движение машинки назад
*/
void moveBack(int time){
    shieldbot.drive(SPEED_WHEELS_ON_MOVEBACK, SPEED_WHEELS_ON_MOVEBACK);
    delay(time);  // Время движения в направлении
}

/* [End init Move] */

/* [Begin Action] */

// Режим ожидания на точке
void actionHold(){
    moveLeftinPlace(DELAY_INPLACE_360);
    delay(1000);
}

/* [End Action] */


/* [Begin controller Move] */

// После определения правильного маршрута, требуется более точная корректировка
// moveRightRoute позволяет подвести машинку в область маркера
void moveRightRoute(double xArea, double yArea, double radiusArea){
  // Проверка принадлежности области к метке
  if(belongsArea(xArea, yArea, radiusArea, xNow, yNow)){
    // Режим ожидания на точке
    actionHold();
    // Переход к новому маркеру, путем выхода из цикла while
    complete_marker = true;
  } else {
    // Точный подъезд к контрольной точке
    if(xNowDelta < yNowDelta){
      //Поворот налево
      moveLeft(DELAY_15);
    } else {
      //Поворот направо
      moveRight(DELAY_15);
    }
  }
}

//Осуществляет определение машинки на поле и основную логику перемещения
void moveTo(double xArea, double yArea, double radiusArea){
  // Цикл обработки, работает до тех пор, пока машинка не доберется до пункта назначения
  while(complete_marker==false){
    Points coordinats = getPosition();
    xNow = coordinats.x;
    yNow = coordinats.y;

    //Получение предыдущих координат
    if(previous_move) {
      xPrevious = coordinats.x;
      yPrevious = coordinats.y;
      previous_move = false;
      //Движение вперед, для получения направления движения
      moveForward(DELAY_FORWARD);
    } else {
      previous_move = true;

      /*
        Координаты на поле не имеют центральных зон с координатами 0, 0
        следовательно delta текущей позиции и delta предыдущей позиции от
        координаты назначенной точки должны быть мудулями.
      */
      /*Растояние от назначенной метки, до предыдущей позиции*/
      xPreviousDelta = abs(xArea - xPrevious);
      yPreviousDelta = abs(yArea - yPrevious);
      /*Растояние от назначенной метки, до текущей позиции*/
      xNowDelta = abs(xArea - xNow);
      yNowDelta = abs(yArea - yNow);
      
      if(xNowDelta < xPreviousDelta) {
        /*
        Объект приближается к цели по оси X
        */
        if(yNowDelta < yPreviousDelta) {
          /*
          Объект приближается к цели по оси Y
          */

          /* [Comment] */ 
          Serial.println("Движение в правильном направлении");
          /* [Comment] */

          //Движение в нужном направлении
          moveRightRoute(xArea, yArea, radiusArea);

        } else {
          /*
          Объект отдоляется от цели по оси Y
          */

          /* [Comment] */ 
          Serial.println("Поворот налево 45");
          /* [Comment] */

          //Поворот налево
          moveLeft(DELAY_45);
        }
      } else {
        /*
        Объект отдоляется от цели по оси X
        */
        if(yNowDelta < yPreviousDelta) {
          /*
          Объект приближается к цели по оси Y
          */

          /* [Comment] */ 
          Serial.println("Поворот направо 45");
          /* [Comment] */

          //Поворот направо
          moveRight(DELAY_45);
        } else {
          /*
          Объект отдоляется от цели по оси Y
          */

          //Движение в обратном направлении от цели
          //Требуется разворот
          /*
          input:
          x1, y1 - координаты Маркера
          x2, y2 - координаты Предыдущей точки
          x3, y3 - координаты Тукущей точки
          output:
          true - поворот направо
          false - поворот налево
          */

          /* [Comment] */ 
          Serial.print("Обратный поворот на 120: ");
          /* [Comment] */

          if(reversalDefinition(xArea, yArea, xPrevious, yPrevious, xNow, yNow)){

          /* [Comment] */ 
          Serial.println("Направо");
          /* [Comment] */

            //Поворот на 120 градусов по часовой стрелке
            moveRightinPlace(DELAY_INPLACE_120);
          } else {

          /* [Comment] */ 
          Serial.println("Налево");
          /* [Comment] */

            //Поворот на 120 градусов против часовой стрелки
            moveLeftinPlace(DELAY_INPLACE_120);
          }
        }
      }
    }
  }
}

/* [End controller Move] */

/* [Begin Programm] */

void setup() {

  /* [Comment] */ 
  Serial.println("Start Setup");
  /* [Comment] */
  
  Bridge.begin();
  shieldbot.setMaxSpeed(100);//255 is max
  // shieldbot.setMaxSpeed(255);//255 is max
  // Initialize Console and wait for port to open:
  setCoordinates();
  //Вывод данных в консоль
  Serial.begin(9600);
}

void loop() {

  /* [Comment] */
  Serial.println("Start Loop");
  /* [Comment] */

  /* В цикле перемещение по координатам точек, начиная со 1-го номера точки*/
  for (point_now; point_now <= POINT_COUNT; point_now++){

    /* [Comment] */
    Serial.print("Точка номер: ");
    Serial.println(point_now);
    /* [Comment] */

    // Вход в цикла while для перемещение к маркеру
    complete_marker = false;

    moveTo(x[point_now], y[point_now], radius[point_now]);
  } 

  /* 
    Возврат к начальной позиции, если последняя точка находится не на финише,
    то выполняем следование к точке финиша (POINT_FINISH). 
   */
  if(point_now!=POINT_FINISH)
    moveTo(x[POINT_FINISH], y[POINT_FINISH], radius[POINT_FINISH]);
  
  
  /* [Comment] */
  Serial.println("Finish Loop");
  /* [Comment] */

  return;
}

/* [End Programm] */
