#include "mbed.h"

#include <cmath>

#include "DA7212.h"


DA7212 audio;

int16_t waveform[kAudioTxBufferSize];

EventQueue queue(32 * EVENTS_EVENT_SIZE);

Thread t;


#define num 49

int song[num];


int noteLength[num];


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


int main(void)

{

  t.start(callback(&queue, &EventQueue::dispatch_forever));


  for(int i = 0; i < num; i++)

  {

    int length = noteLength[i];

    while(length--)

    {

      queue.call(playNote, song[i]);

      if(length <= 1) wait(1.0);

    }

  }

}


// Here are the songs

/*---twinkel twinkel little star---

  #define num 25

  int song[42] = {

    261, 261, 392, 392, 440, 440, 392,

    349, 349, 330, 330, 294, 294, 261,

    392, 392, 349, 349, 330, 330, 294,

    392, 392, 349, 349, 330, 330, 294,

    261, 261, 392, 392, 440, 440, 392,

    349, 349, 330, 330, 294, 294, 261};


  int noteLength[42] = {

    1, 1, 1, 1, 1, 1, 2,

    1, 1, 1, 1, 1, 1, 2,

    1, 1, 1, 1, 1, 1, 2,

    1, 1, 1, 1, 1, 1, 2,

    1, 1, 1, 1, 1, 1, 2,

    1, 1, 1, 1, 1, 1, 2};*/


/*---happy birthday---

  #define num 25

  int song[num] = {

    261, 261, 293, 261, 349, 329,

    261, 261, 293, 261, 391, 349,

    261, 261, 523, 440, 349, 329,

    293, 466, 466, 440, 349, 392,

    349};


  int noteLength[num] = {

    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1};*/


/*---Old MacDonald Had a farm---

#define num 49

int song[num] = {

  523, 523, 523, 392, 440, 440,

  392, 659, 659, 587, 587, 523,

  391, 523, 523, 523, 391,
  
  440, 440, 391, 659, 659,
  
  587, 587, 523, 391, 523,
  
  523, 523, 391, 523, 523, 523,
  
  523, 523, 523, 391, 440,
    
  440, 391, 659, 659, 587, 587,
  
  523};


int noteLength[num] = {

  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1};*/

