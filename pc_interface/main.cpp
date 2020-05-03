#include "mbed.h"
#include <cmath>
#include "DA7212.h"
#include "uLCD_4DGL.h"


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
InterruptIn sw2(SW2);
//InterruptIn sw3(SW3);
EventQueue queue(32 * EVENTS_EVENT_SIZE);
Thread t;
int idC = 0;
int16_t waveform[kAudioTxBufferSize];


// interface
int mod_sel = 2;
int song_sel = 1;
bool flag_song = true;
bool flag_mode = true;


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


// functions declaration
void loadMusic(void);
void playNote(int freq);
void playMusic(void);
void uLCD_mode(void);
void uLCD_song(void);
void clearBuffer(void);
void showMusic(void);
int STOI(char *, int);



int main(void)
{
  green_led = 1;


  t.start(callback(&queue, &EventQueue::dispatch_forever));

  //sw2.rise(queue.event(loadMusicHandler));

  //sw3.rise(queue.event(playMusicHandler));

  //sw3.fall(queue.event(stopPlayMusic));
  

  while (1) // main program loop
  {
    // mode selection

    uLCD_mode();

    if (mod_sel == 0 && Switch == 0)
    {
      pc.printf("mode 1 selected\r\n");
    }
    else if (mod_sel == 1 && Switch == 0)
    {
      pc.printf("mode 2 selected\r\n");
    }
    else if (mod_sel == 2 && Switch == 0)
    {
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
          pc.printf("%s\r\n", songList[song_sel]); // write song name to python

          loadMusic();

          flag = false;

          break;
        }


      }
      

    }

  }


}


// fuctions definition

void loadMusic(void)
{
  green_led = 0; 

  audio.spk.pause();

  //t.terminate();

  queue.cancel(idC);

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
  wait(1.0);

  idC = queue.call_every(noteNum, playMusic);

  //t.start(playMusic);

  //t.join();

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
  while(1)

    for(int i = 0; i < noteNum; i++)

    {

      int length = noteLength[i];

      while(length--)

      {

        playNote(note[i]);

        if(length <= 1) wait(1.0);

      }

    }

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

  uLCD.locate(1, 3);

  uLCD.printf("\n   2. backward\n");

  uLCD.locate(1, 4);

  uLCD.printf("\n   3. change song\n");

  uLCD.locate(1, 8);

  uLCD.printf("\n  Current song : \n");

  uLCD.locate(1, 9);

  uLCD.printf("\n%s\n", name);

  uLCD.locate(1, mod_sel + 3);

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

  uLCD.locate(1, 2);

  uLCD.printf("\n   1. %s\n", songList[0]);

  uLCD.locate(1, 4);

  uLCD.printf("\n   2. %s\n", songList[1]);

  uLCD.locate(1, 6);

  uLCD.printf("\n   3. %s\n", songList[2]);

  uLCD.locate(1, song_sel * 2 + 3);

  uLCD.printf("->");

  flag_mode = true; 

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
