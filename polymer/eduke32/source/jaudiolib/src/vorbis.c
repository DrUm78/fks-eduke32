/*
 Copyright (C) 2009 Jonathon Fowler <jf@jonof.id.au>
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 
 See the GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 */

/**
 * OggVorbis source support for MultiVoc
 */

#ifdef HAVE_VORBIS

#ifdef __APPLE__
# include <vorbis/vorbisfile.h>
#else
# include "vorbis/vorbisfile.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <errno.h>
#include "pitch.h"
#include "multivoc.h"
#include "_multivc.h"

#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif

#define BLOCKSIZE 0x8000

typedef struct {
   void * ptr;
   size_t length;
   size_t pos;
   
   OggVorbis_File vf;
   
   char block[BLOCKSIZE];
   int32_t lastbitstream;
} vorbis_data;


static size_t read_vorbis(void * ptr, size_t size, size_t nmemb, void * datasource)
{
   vorbis_data * vorb = (vorbis_data *) datasource;
   size_t nread = 0;
   size_t bytes;
   
   errno = 0;

   if (vorb->length == vorb->pos) {
      return 0;
   }
   
   for (; nmemb > 0; nmemb--, nread++) {
      bytes = vorb->length - vorb->pos;
      if (size < bytes) {
         bytes = size;
      }
      
      memcpy(ptr, (uint8_t *)vorb->ptr + vorb->pos, bytes);
      vorb->pos += bytes;
      ptr = (uint8_t *)ptr + bytes;
      
      if (vorb->length == vorb->pos) {
         nread++;
         break;
      }
   }
   
   return nread;
}


static int32_t seek_vorbis(void * datasource, ogg_int64_t offset, int32_t whence)
{
   vorbis_data * vorb = (vorbis_data *) datasource;
   
   switch (whence) {
      case SEEK_SET: vorb->pos = 0; break;
      case SEEK_CUR: break;
      case SEEK_END: vorb->pos = vorb->length; break;
   }
   
   vorb->pos += offset;

   if (vorb->pos > vorb->length) {
      vorb->pos = vorb->length;
   }
   
   return vorb->pos;
}

static int32_t close_vorbis(void * datasource)
{
    UNREFERENCED_PARAMETER(datasource);
   return 0;
}

static long tell_vorbis(void * datasource)
{
   vorbis_data * vorb = (vorbis_data *) datasource;
   
   return vorb->pos;
}

static ov_callbacks vorbis_callbacks = {
   read_vorbis,
   seek_vorbis,
   close_vorbis,
   tell_vorbis
};


/*---------------------------------------------------------------------
Function: MV_GetNextVorbisBlock

Controls playback of OggVorbis data
---------------------------------------------------------------------*/

static playbackstatus MV_GetNextVorbisBlock
(
 VoiceNode *voice
 )

{
   vorbis_data * vd = (vorbis_data *) voice->extra;
   int32_t bytes, bytesread;
   int32_t bitstream, err;

   voice->Playing = TRUE;
   
   bytesread = 0;
   do {
       bytes = ov_read(&vd->vf, vd->block + bytesread, BLOCKSIZE - bytesread, 0, 2, 1, &bitstream);
       //fprintf(stderr, "ov_read = %d\n", bytes);
       if (bytes > 0) {
           bytesread += bytes;
           if ((ogg_int64_t)(intptr_t)voice->LoopEnd > 0
                && ov_pcm_tell(&vd->vf) >= (ogg_int64_t)(intptr_t)voice->LoopEnd) {
                   err = ov_pcm_seek(&vd->vf,(ogg_int64_t)(intptr_t)voice->LoopStart);
                   if (err != 0) {
                       MV_Printf("MV_GetNextVorbisBlock ov_pcm_seek: LOOP_START %l, LOOP_END %l, err %d\n",
                           (ogg_int64_t)(intptr_t)voice->LoopStart, (ogg_int64_t)(intptr_t)voice->LoopEnd, err);
                   } else {
                       continue;
                   }
               }
            continue;
       }
       else if (bytes == OV_HOLE) continue;
       else if (bytes == 0) {
           if (voice->LoopSize > 0) {
               err = ov_pcm_seek(&vd->vf,(ogg_int64_t)(intptr_t)voice->LoopStart);
               if (err != 0) {
                   MV_Printf("MV_GetNextVorbisBlock ov_pcm_seek: LOOP_START %l, err %d\n",
                       (ogg_int64_t)(intptr_t)voice->LoopStart, err);
               } else {
                   continue;
               }
           } else {
               break;
           }
       } else if (bytes < 0) {
           MV_Printf("MV_GetNextVorbisBlock ov_read: err %d\n", bytes);
           voice->Playing = FALSE;
           return NoMoreData;
       }
   } while (bytesread < BLOCKSIZE);

   if (bytesread == 0) {
      voice->Playing = FALSE;
      return NoMoreData;
   }
      
   if (bitstream != vd->lastbitstream) {
      vorbis_info * vi = 0;
      
      vi = ov_info(&vd->vf, -1);
      if (!vi || (vi->channels != 1 && vi->channels != 2)) {
         voice->Playing = FALSE;
         return NoMoreData;
      }
      
      voice->channels = vi->channels;
      voice->SamplingRate = vi->rate;
      voice->RateScale    = ( voice->SamplingRate * voice->PitchScale ) / MV_MixRate;
      voice->FixedPointBufferSize = ( voice->RateScale * MixBufferSize ) - voice->RateScale;
      MV_SetVoiceMixMode( voice );
      vd->lastbitstream = bitstream;
   }

   bytesread /= 2 * voice->channels;
   
   voice->position    = 0;
   voice->sound       = vd->block;
   voice->BlockLength = 0;
   voice->length      = bytesread << 16;
   
   return( KeepPlaying );
}


/*---------------------------------------------------------------------
Function: MV_PlayVorbis3D

Begin playback of sound data at specified angle and distance
from listener.
---------------------------------------------------------------------*/

int32_t MV_PlayVorbis3D
(
 char *ptr,
 uint32_t ptrlength,
 int32_t  pitchoffset,
 int32_t  angle,
 int32_t  distance,
 int32_t  priority,
 uint32_t callbackval
 )

{
   int32_t left;
   int32_t right;
   int32_t mid;
   int32_t volume;
   int32_t status;
   
   if ( !MV_Installed )
   {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
   }
   
   if ( distance < 0 )
   {
      distance  = -distance;
      angle    += MV_NumPanPositions / 2;
   }
   
   volume = MIX_VOLUME( distance );
   
   // Ensure angle is within 0 - 31
   angle &= MV_MaxPanPosition;
   
   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );
   
   status = MV_PlayVorbis( ptr, ptrlength, pitchoffset, mid, left, right, priority,
                           callbackval );
   
   return( status );
}


/*---------------------------------------------------------------------
Function: MV_PlayVorbis

Begin playback of sound data with the given sound levels and
priority.
---------------------------------------------------------------------*/

int32_t MV_PlayVorbis
(
 char *ptr,
 uint32_t ptrlength,
 int32_t   pitchoffset,
 int32_t   vol,
 int32_t   left,
 int32_t   right,
 int32_t   priority,
 uint32_t callbackval
 )

{
   int32_t status;
   
   status = MV_PlayLoopedVorbis( ptr, ptrlength, -1, -1, pitchoffset, vol, left, right,
                                 priority, callbackval );
   
   return( status );
}


/*---------------------------------------------------------------------
Function: MV_PlayLoopedVorbis

Begin playback of sound data with the given sound levels and
priority.
---------------------------------------------------------------------*/

int32_t MV_PlayLoopedVorbis
(
 char *ptr,
 uint32_t ptrlength,
 int32_t   loopstart,
 int32_t   loopend,
 int32_t   pitchoffset,
 int32_t   vol,
 int32_t   left,
 int32_t   right,
 int32_t   priority,
 uint32_t callbackval
 )

{
   VoiceNode   *voice;
   int32_t          status;
   vorbis_data * vd = 0;
   vorbis_info * vi = 0;

   if ( !MV_Installed )
   {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
   }
   
   vd = (vorbis_data *) malloc( sizeof(vorbis_data) );
   if (!vd) {
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   memset(vd, 0, sizeof(vorbis_data));
   vd->ptr = ptr;
   vd->pos = 0;
   vd->length = ptrlength;
   vd->lastbitstream = -1;
   
   status = ov_open_callbacks((void *) vd, &vd->vf, 0, 0, vorbis_callbacks);
   if (status < 0) {
      free(vd);
      MV_Printf("MV_PlayLoopedVorbis: err %d\n", status);
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   vi = ov_info(&vd->vf, 0);
   if (!vi) {
      ov_clear(&vd->vf);
      free(vd);
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   if (vi->channels != 1 && vi->channels != 2) {
      ov_clear(&vd->vf);
      free(vd);
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   // Request a voice from the voice pool
   voice = MV_AllocVoice( priority );
   if ( voice == NULL )
   {
      ov_clear(&vd->vf);
      free(vd);
      MV_SetErrorCode( MV_NoVoices );
      return( MV_Error );
   }
   
   voice->wavetype    = Vorbis;
   voice->bits        = 16;
   voice->channels    = vi->channels;
   voice->extra       = (void *) vd;
   voice->GetSound    = MV_GetNextVorbisBlock;
   voice->NextBlock   = vd->block;
   voice->DemandFeed  = NULL;
   voice->LoopCount   = 0;
   voice->BlockLength = 0;
   voice->PitchScale  = PITCH_GetScale( pitchoffset );
   voice->length      = 0;
   voice->next        = NULL;
   voice->prev        = NULL;
   voice->priority    = priority;
   voice->callbackval = callbackval;

   voice->LoopStart   = (char *) (intptr_t)(loopstart >= 0 ? loopstart : 0);
   voice->LoopEnd     = (char *) (intptr_t)(loopstart >= 0 && loopend > 0 ? loopend : 0);
   voice->LoopSize    = (loopstart >= 0 ? 1 : 0);

    // load loop tags from metadata
    if (loopstart < 1)
    {
        vorbis_comment * vc = ov_comment(&vd->vf, 0);
        if (vc != NULL)
        {
            int loopTagCount;

            static const char *loopStartTags[] = {
                "LOOP_START",
                "LOOPSTART"
            };

            char *vc_loopstart = NULL;

            for (loopTagCount = 0; loopTagCount < 2 && vc_loopstart == NULL; ++loopTagCount)
                vc_loopstart = vorbis_comment_query(vc, loopStartTags[loopTagCount], 0);

            if (vc_loopstart != NULL)
            {
                ogg_int64_t ov_loopstart = atol(vc_loopstart);
                if (ov_loopstart >= 0) // a loop starting at 0 is valid
                {
                    static const char *loopEndTags[] = {
                        "LOOP_END",
                        "LOOPEND"
                    };

                    char *vc_loopend = NULL;

                    voice->LoopStart = (char *) (intptr_t) ov_loopstart;
                    voice->LoopSize = 1;

                    for (loopTagCount = 0; loopTagCount < 2 && vc_loopend == NULL; ++loopTagCount)
                        vc_loopend = vorbis_comment_query(vc, loopEndTags[loopTagCount], 0);

                    if (vc_loopend != NULL)
                    {
                        ogg_int64_t ov_loopend = atol(vc_loopend);
                        if (ov_loopend > 0) // a loop ending at 0 is invalid
                            voice->LoopEnd = (char *) (intptr_t) ov_loopend;
                    }
                    else
                    {
                        static const char *loopLengthTags[] = {
                            "LOOP_LENGTH",
                            "LOOPLENGTH"
                        };

                        char *vc_looplength = NULL;

                        for (loopTagCount = 0; loopTagCount < 2 && vc_looplength == NULL; ++loopTagCount)
                            vc_looplength = vorbis_comment_query(vc, loopLengthTags[loopTagCount], 0);

                        if (vc_looplength != NULL)
                        {
                            ogg_int64_t ov_looplength = atol(vc_looplength);
                            if (ov_looplength > 0) // a loop of length 0 is invalid
                                voice->LoopEnd = (char *) (intptr_t) (ov_looplength + ov_loopstart);
                        }
                    }
                }
            }
        }
    }

   voice->Playing     = TRUE;
   voice->Paused      = FALSE;
   
   voice->SamplingRate = vi->rate;
   voice->RateScale    = ( voice->SamplingRate * voice->PitchScale ) / MV_MixRate;
   voice->FixedPointBufferSize = ( voice->RateScale * MixBufferSize ) -
      voice->RateScale;
   MV_SetVoiceMixMode( voice );

   MV_SetVoiceVolume( voice, vol, left, right );
   MV_PlayVoice( voice );
   
   return( voice->handle );
}


void MV_ReleaseVorbisVoice( VoiceNode * voice )
{
   vorbis_data * vd = (vorbis_data *) voice->extra;
   
   if (voice->wavetype != Vorbis) {
      return;
   }
   
   ov_clear(&vd->vf);
   free(vd);
   
   voice->extra = 0;
}

#endif //HAVE_VORBIS
