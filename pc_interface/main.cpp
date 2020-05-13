#include "mbed.h"
#include <cmath>
#include "DA7212.h"
#include "uLCD_4DGL.h"

//------------DNL----------------------
#include "accelerometer_handler.h"
#include "config.h"
#include "magic_wand_model_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
//------------DNL----------------------

//------------tilt---------------------
#include "fsl_port.h"

#include "fsl_gpio.h"

#define UINT14_MAX        16383

// FXOS8700CQ I2C address

#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0

#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0

#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1

#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1

// FXOS8700CQ internal register addresses

#define FXOS8700Q_STATUS 0x00

#define FXOS8700Q_OUT_X_MSB 0x01

#define FXOS8700Q_OUT_Y_MSB 0x03

#define FXOS8700Q_OUT_Z_MSB 0x05

#define FXOS8700Q_M_OUT_X_MSB 0x33

#define FXOS8700Q_M_OUT_Y_MSB 0x35

#define FXOS8700Q_M_OUT_Z_MSB 0x37

#define FXOS8700Q_WHOAMI 0x0D

#define FXOS8700Q_XYZ_DATA_CFG 0x0E

#define FXOS8700Q_CTRL_REG1 0x2A

#define FXOS8700Q_M_CTRL_REG1 0x5B

#define FXOS8700Q_M_CTRL_REG2 0x5C

#define FXOS8700Q_WHOAMI_VAL 0xC7

I2C i2c1( PTD9,PTD8);

int m_addr = FXOS8700CQ_SLAVE_ADDR1;

void FXOS8700CQ_readRegs1(int addr, uint8_t * data, int len);

void FXOS8700CQ_writeRegs1(uint8_t * data, int len);
//------------tilt---------------------


#define bufferLength (32)
#define musiclLength (100)
#define nameLength (256)
#define listMaxLength (20)

// settings
DA7212 audio;
Serial pc(USBTX, USBRX);
DigitalOut green_led(LED2);;
DigitalIn  Switch(SW3);
uLCD_4DGL uLCD(D1, D0, D2);
EventQueue queue(32 * EVENTS_EVENT_SIZE);
Thread t;
Thread t1(osPriorityNormal, 120 * 1024 /*120K stack size*/);
int idC = 0;
int16_t waveform[kAudioTxBufferSize];


// interface
int mod_sel = 0;
bool mod_sel_enable = false;
int song_sel = 0;
bool song_sel_enable = false;
bool flag_song = true;
bool flag_mode = true;
bool flag_audio_stop = false;
int cnt = 0;



// music data
char serialInBuffer[bufferLength];
char c;
int serialCount = 0;
//---------------------------------------------
char name[nameLength] = "No song is playing";
int noteNum = 0;
int note[musiclLength];
int noteLength[musiclLength];
//---------------------------------------------
char songList[listMaxLength][nameLength];
int listLength = 0;
bool flag = true;
float magnitude_x = 0;
float magnitude_y = 0;



// functions declaration
void loadMusic(void);
void playNote(int freq);
void playMusic(void);
void uLCD_mode(void);
void uLCD_song(void);
void uLCD_Taiko(void);
void clearBuffer(void);
void showMusic(void);
int STOI(char *, int);
int PredictGesture(float*);
void Predict_init(void);
void Predict(void);
bool beat_capture(void);



int main(void)
{
  green_led = 1;

  t.start(callback(&queue, &EventQueue::dispatch_forever));

  t1.start(Predict);


  while (1) // main program loop
  {
    // mode selection

    mod_sel_enable = true; // DNL

    song_sel_enable = false; // DNL

    song_sel = 0;

    uLCD_mode();


    if (mod_sel == 0 && Switch == 0)
    {

      if (cnt + 5 < noteNum)
        cnt += 5;
      else
        cnt = noteNum;

    }
    else if (mod_sel == 1 && Switch == 0)
    {

      if (cnt - 5 > 0)
        cnt -= 5;
      else
        cnt = 0;

    }
    else if (mod_sel == 2 && Switch == 0)
    {
      mod_sel_enable = false; // DNL

      song_sel_enable = true; // DNL

      song_sel = 0;

      if (flag)
        pc.printf("read song list\r\n"); // write string to python

      // receive listLength from python
      while (serialCount < 4 & flag)  // get listLength
      {

        if(pc.readable())
        {

          c = pc.getc();

          if(c != '-')
          {

            serialInBuffer[serialCount] = c;

            serialCount++;

          }
          else
          {

            listLength = STOI(serialInBuffer, serialCount);

            serialCount = 0;

            break;

          }

        }

      }
      
      // receive songList from python
      for (int i = 0; i < listLength & flag; ++i)
      {
        while(serialCount < nameLength) // get name
        {
          if(pc.readable())
          {
            if ((c =  pc.getc()) == '-') break;

            songList[i][serialCount] = c;

            serialCount++;

          }
        }
        serialCount = 0;
      }

      while (1) // song selection
      {
        uLCD_song();

        if (Switch == 0)
        {
          pc.baud(9600);

          pc.printf("%s\r\n", songList[song_sel]); // write song name to python

          //---load music---

          flag_audio_stop = true;

          loadMusic();

          wait(2.0);

          flag_audio_stop = false;

          queue.call(playMusic);

          //---load music---
          
          mod_sel = 0;

          flag = false;

          break;
        }


      }
      

    }
    else if (mod_sel == 3 && Switch == 0)
    {
      // Taiko game mode
      // if the next note is higher than the current note, then the beat is strong
      // if the next note is lower than the current note, then the beat is weak
      // if the next note is same as the current note, then the beat is the same

      uLCD.cls();

      flag_audio_stop = true;

      wait(1.0);

      flag_audio_stop = false;

      queue.call(playMusic);

      uLCD_Taiko();

      flag_mode = true;
      
    }

  }


}


// fuctions definition

void loadMusic(void)
{
  green_led = 0; 

  serialCount = 0;


  while(serialCount < nameLength) // get name
  {
    if(pc.readable())
    {
      c =  pc.getc();

      if (c == '-') break;

      name[serialCount] = c;

      serialCount++;

    }
  }

  name[serialCount] = NULL;


  serialCount = 0;

  while (serialCount < 4)  // get noteNum
  {

    if(pc.readable())
    {

      c = pc.getc();

      if(c != '-')
      {

        serialInBuffer[serialCount] = c;

        serialCount++;

      }
      else
      {

        noteNum = STOI(serialInBuffer, serialCount);

        serialCount = 0;

        break;

      }

    }

  }


  for (int i = 0; i < noteNum;)  // get note
  {

    if(pc.readable())
    {

      c = pc.getc();

      if (c == '-') break;

      if(c != ',')
      {

        serialInBuffer[serialCount] = c;

        serialCount++;

      }
      else
      {
        

        note[i] = STOI(serialInBuffer, serialCount);

        serialCount = 0;

        i++;
        
      }

    }

  }

  c = pc.getc();

  for (int i = 0; i < noteNum;)  // get length
  {

    if(pc.readable())
    {

      c = pc.getc();

      if (c == '-') break;

      if(c != ',')
      {

        noteLength[i] = (int) c - 48;

      }
      else
      {

        i++;
        
      }

    }

  }

  green_led = 1;

}

void playNote(int freq)

{

  for (int i = 0; i < kAudioTxBufferSize; i++)

  {

    waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / freq)) * ((1<<16) - 1));

  }

  // the loop below will play the note for the duration of 1s

  for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j)

  {

    audio.spk.play(waveform, kAudioTxBufferSize);

  }

}

void playMusic(void) 
{

  for(cnt = 0; cnt < noteNum & !flag_audio_stop; cnt++)
  {

    int length = noteLength[cnt];

    while(length-- & !flag_audio_stop)

    {

      playNote(note[cnt]);

      if(length <= 1) wait(1.0);

    }

  }

  playNote(0);

}

void uLCD_mode()
{
  if (flag_mode == true)
  {
    uLCD.cls();
    flag_mode = false;
  }

  uLCD.locate(1, 2);

  uLCD.printf("\n   1. forward\n");

  uLCD.locate(1, 4);

  uLCD.printf("\n   2. backward\n");

  uLCD.locate(1, 6);

  uLCD.printf("\n   3. change song\n");

  uLCD.locate(1, 8);

  uLCD.printf("\n   4. Taiko game\n");

  uLCD.locate(1, 10);

  uLCD.printf("\n  Current song : \n");

  uLCD.locate(1, 11);

  uLCD.printf("\n%s\n", name);

  uLCD.locate(1, 2 * mod_sel + 3);

  uLCD.printf("->");

  flag_song = true;
}

void uLCD_song()
{
  if (flag_song == true)
  {
    uLCD.cls();
    flag_song = false;
  }

  uLCD.locate(1, 5);

  uLCD.printf("  select a song:");

  uLCD.locate(1, 7);

  uLCD.printf("%s", songList[song_sel]);

  uLCD.printf("                   "
              "                   ");  //  clear the screen

  flag_mode = true; 

}

void uLCD_Taiko(void)
{
  uLCD.locate(1, 5);

  uLCD.printf("  next note:");

  bool beat = false;

  float score = 0; // ranging from 0 to 10

  while (cnt < noteNum)
  {
    uLCD.locate(1, 6);

    if (note[cnt + 1] > note[cnt])
      beat = true;
    else if (note[cnt + 1] < note[cnt])
      beat = false;
    
    if (beat)
      uLCD.printf("  strong");
    else
      uLCD.printf("  weak  ");

    uLCD.printf("\n    %f", magnitude_x);

    uLCD.printf("\n    %f", magnitude_y);
    

    if (beat == beat_capture())
      score++;
  }

  // show score for 2 sec. then back to main menu

  uLCD.cls();

  uLCD.locate(1, 5);

  uLCD.printf("  total score:\n     %d", int(score / noteNum * 10));

  wait(2.0);
  
  mod_sel = 0;

}

void showMusic(void)
{
  pc.printf("%s\r\n", name);

  pc.printf("%d\r\n", noteNum);

  for (int i = 0; i < noteNum; ++i) {
    pc.printf("%d / ", note[i]);
  }

  pc.printf("\r\n");

  for (int i = 0; i < noteNum; ++i) {
    pc.printf("%d / ", noteLength[i]);
  }

}

int STOI(char * buffer, int len)
{
  char replica[len + 1];

  for (int i = 0; i < len; ++i)
  {
    replica[i] = buffer[i];
  }

  return atoi(replica);
}


// Return the result of the last prediction

int PredictGesture(float* output) {

  // How many times the most recent gesture has been matched in a row

  static int continuous_count = 0;

  // The result of the last prediction

  static int last_predict = -1;


  // Find whichever output has a probability > 0.8 (they sum to 1)

  int this_predict = -1;

  for (int i = 0; i < label_num; i++) {

    if (output[i] > 0.8) this_predict = i;

  }


  // No gesture was detected above the threshold

  if (this_predict == -1) {

    continuous_count = 0;

    last_predict = label_num;

    return label_num;

  }


  if (last_predict == this_predict) {

    continuous_count += 1;

  } else {

    continuous_count = 0;

  }

  last_predict = this_predict;


  // If we haven't yet had enough consecutive matches for this gesture,

  // report a negative result

  if (continuous_count < config.consecutiveInferenceThresholds[this_predict]) {

    return label_num;

  }

  // Otherwise, we've seen a positive result, so clear all our variables

  // and report it

  continuous_count = 0;

  last_predict = -1;


  return this_predict;

}




void Predict(void)
{

  // Create an area of memory to use for input, output, and intermediate arrays.

  // The size of this will depend on the model you're using, and may need to be

  // determined by experimentation.

  constexpr int kTensorArenaSize = 60 * 1024;  

  uint8_t tensor_arena[kTensorArenaSize];


  // Whether we should clear the buffer next time we fetch data

  bool should_clear_buffer = false;

  bool got_data = false;


  // The gesture index of the prediction

  int gesture_index;


  // Set up logging.

  static tflite::MicroErrorReporter micro_error_reporter;

  tflite::ErrorReporter* error_reporter = &micro_error_reporter;


  // Map the model into a usable data structure. This doesn't involve any

  // copying or parsing, it's a very lightweight operation.

  const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);

  if (model->version() != TFLITE_SCHEMA_VERSION) {

    error_reporter->Report(

        "Model provided is schema version %d not equal "

        "to supported version %d.",

        model->version(), TFLITE_SCHEMA_VERSION);

    
    return;

  }


  // Pull in only the operation implementations we need.

  // This relies on a complete list of all the ops needed by this graph.

  // An easier approach is to just use the AllOpsResolver, but this will

  // incur some penalty in code space for op implementations that are not

  // needed by this graph.

  static tflite::MicroOpResolver<6> micro_op_resolver;


  micro_op_resolver.AddBuiltin(

      tflite::BuiltinOperator_DEPTHWISE_CONV_2D,

      tflite::ops::micro::Register_DEPTHWISE_CONV_2D());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,

                               tflite::ops::micro::Register_MAX_POOL_2D());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,

                               tflite::ops::micro::Register_CONV_2D());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,

                               tflite::ops::micro::Register_FULLY_CONNECTED());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,

                               tflite::ops::micro::Register_SOFTMAX());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
                               tflite::ops::micro::Register_RESHAPE(), 1);


  // Build an interpreter to run the model with

  static tflite::MicroInterpreter static_interpreter(

      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);

  tflite::MicroInterpreter* interpreter = &static_interpreter;


  // Allocate memory from the tensor_arena for the model's tensors

  interpreter->AllocateTensors();


  // Obtain pointer to the model's input tensor

  TfLiteTensor* model_input = interpreter->input(0);

  if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||

      (model_input->dims->data[1] != config.seq_length) ||

      (model_input->dims->data[2] != kChannelNumber) ||

      (model_input->type != kTfLiteFloat32)) {

    error_reporter->Report("Bad input tensor parameters in model");

    
    return;

  }


  int input_length = model_input->bytes / sizeof(float);


  TfLiteStatus setup_status = SetupAccelerometer(error_reporter);

  if (setup_status != kTfLiteOk) {

    error_reporter->Report("Set up failed\n");

    
    return;

  }


  error_reporter->Report("Set up successful...\n");




  while (true) 
  {
    if (flag_audio_stop) wait(2.0);

    // Attempt to read new data from the accelerometer

    got_data = ReadAccelerometer(error_reporter, model_input->data.f,

                                 input_length, should_clear_buffer);


    // If there was no new data,

    // don't try to clear the buffer again and wait until next time

    if (!got_data) {

      should_clear_buffer = false;

      continue;

    }


    // Run inference, and report any error

    TfLiteStatus invoke_status = interpreter->Invoke();

    if (invoke_status != kTfLiteOk) {

      error_reporter->Report("Invoke failed on index: %d\n", begin_index);

      continue;

    }


    // Analyze the results to obtain a prediction

    gesture_index = PredictGesture(interpreter->output(0)->data.f);


    // Clear the buffer next time we read data

    should_clear_buffer = gesture_index < label_num;


    // Produce an output

    if (gesture_index < label_num) {

      error_reporter->Report(config.output_message[gesture_index]);

      if (mod_sel_enable)
      {
        if (gesture_index == 0)
          ++mod_sel;
        else if (gesture_index == 1)
          --mod_sel;
      }
      else if (song_sel_enable)
      {
        if (gesture_index == 0)
          ++song_sel;
        else if (gesture_index == 1)
          --song_sel;
      }

      if (mod_sel == 4) mod_sel = 0; // barreling

      if (mod_sel == -1) mod_sel = 3; // barreling

      if (song_sel == listLength) song_sel = 0; // barreling

      if (song_sel == -1) song_sel = listLength - 1; // barreling
      
    }


  }

}

bool beat_capture(void)
{

  pc.baud(115200);


  uint8_t who_am_i, data[2], res[6];

  int16_t acc16;

  float t[3];


  // Enable the FXOS8700Q


  FXOS8700CQ_readRegs1( FXOS8700Q_CTRL_REG1, &data[1], 1);

  data[1] |= 0x01;

  data[0] = FXOS8700Q_CTRL_REG1;

  FXOS8700CQ_writeRegs1(data, 2);


  // Get the slave address

  FXOS8700CQ_readRegs1(FXOS8700Q_WHOAMI, &who_am_i, 1);




// while true

    FXOS8700CQ_readRegs1(FXOS8700Q_OUT_X_MSB, res, 6);


    acc16 = (res[0] << 6) | (res[1] >> 2);

    if (acc16 > UINT14_MAX/2)

        acc16 -= UINT14_MAX;

    t[0] = ((float)acc16) / 4096.0f;


    acc16 = (res[2] << 6) | (res[3] >> 2);

    if (acc16 > UINT14_MAX/2)

        acc16 -= UINT14_MAX;

    t[1] = ((float)acc16) / 4096.0f;


    acc16 = (res[4] << 6) | (res[5] >> 2);

    if (acc16 > UINT14_MAX/2)

        acc16 -= UINT14_MAX;

    t[2] = ((float)acc16) / 4096.0f;

    magnitude_x = t[0];

    magnitude_y = t[1];

    if (t[0] > 0.75 || t[1] > 0.75 || t[0] < -0.75 || t[1] < -0.75) return true;
    else return false;


}


void FXOS8700CQ_readRegs1(int addr, uint8_t * data, int len) {

  char t = addr;

  i2c1.write(m_addr, &t, 1, true);

  i2c1.read(m_addr, (char *)data, len);

}


void FXOS8700CQ_writeRegs1(uint8_t * data, int len) {

  i2c1.write(m_addr, (char *)data, len);

}


