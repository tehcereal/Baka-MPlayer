#include "mpvhandler.h"

#include <QCoreApplication>
#include <string>

static void wakeup(void *ctx)
{
    MpvHandler *mpvhandler = (MpvHandler*)ctx;
    QCoreApplication::postEvent(mpvhandler, new QEvent(QEvent::User));
}

MpvHandler::MpvHandler(int64_t wid, QObject *parent):
    QObject(parent),
    mpv(0),
    file(""),
    time(0),
    totalTime(0),
    volume(100),
    playState(Mpv::Stopped)
{
    mpv = mpv_create();
    if(!mpv)
        throw "Could not create mpv object";

    mpv_set_option(mpv, "wid", MPV_FORMAT_INT64, &wid);

    mpv_set_option_string(mpv, "input-default-bindings", "yes"); // mpv default key bindings
//    mpv_set_option_string(mpv, "osd", "no"); // no onscreen display
//    mpv_set_option_string(mpv, "osc", "no"); // no onscreen controller

    // customize keybindings and seek values etc...

    mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "length", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "path", MPV_FORMAT_STRING);

    mpv_set_wakeup_callback(mpv, wakeup, this);

    if(mpv_initialize(mpv) < 0)
        throw "Could not initialize mpv";
}

MpvHandler::~MpvHandler()
{
    if(mpv)
    {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
    }
}

bool MpvHandler::event(QEvent *event)
{
    if(event->type() == QEvent::User)
    {
        while(mpv)
        {
            mpv_event *event = mpv_wait_event(mpv, 0);
            if (event->event_id == MPV_EVENT_NONE)
                break;
            switch (event->event_id)
            {
            case MPV_EVENT_PROPERTY_CHANGE:
            {
                mpv_event_property *prop = (mpv_event_property*)event->data;
                if (strcmp(prop->name, "time-pos") == 0)
                    if (prop->format == MPV_FORMAT_DOUBLE)
                        SetTime((time_t)*(double*)prop->data);
                if (strcmp(prop->name, "length") == 0)
                    if (prop->format == MPV_FORMAT_DOUBLE)
                        SetTotalTime((time_t)*(double*)prop->data);
                if (strcmp(prop->name, "volume") == 0)
                    if (prop->format == MPV_FORMAT_DOUBLE)
                        SetVolume((int)*(double*)prop->data);
                if (strcmp(prop->name, "path") == 0)
                    if (prop->format == MPV_FORMAT_STRING)
                        SetFile(((std::string*)prop->data)->c_str());
                break;
            }
            case MPV_EVENT_FILE_LOADED: // essentially when i get this event it won't start
                break;
            case MPV_EVENT_IDLE:
                SetTime(0);
                SetPlayState(Mpv::Idle);
                break;
            case MPV_EVENT_START_FILE:
                SetPlayState(Mpv::Started);
            case MPV_EVENT_UNPAUSE:
                SetPlayState(Mpv::Playing);
                break;
            case MPV_EVENT_PAUSE:
                SetPlayState(Mpv::Paused);
                break;
            case MPV_EVENT_END_FILE:
                SetPlayState(Mpv::Ended);
                SetPlayState(Mpv::Stopped);
                break;
            case MPV_EVENT_SHUTDOWN:
                QCoreApplication::quit();
                break;
            case MPV_EVENT_COMMAND_REPLY:
                if(event->error != MPV_ERROR_SUCCESS)
                    emit ErrorSignal(QString::number(event->error));
                break;
            default:
                // unhandled events
                break;
            }
        }
        return true;
    }
    return QObject::event(event);
}

void MpvHandler::CloseFile()
{
    if(mpv)
    {
        const char *args[] = {"playlist_remove", "current", NULL};
        mpv_command_async(mpv, 0, args);
    }
    else
        emit ErrorSignal("mpv was not initialized");
}

void MpvHandler::OpenFile(QString f/*, QString subFile = ""*/)
{
    if(mpv)
    {
//        externalSub = subFile;
        if(playState == Mpv::Playing || playState == Mpv::Started)
            CloseFile();
        const char *args[] = {"loadfile", f.toUtf8().data(), NULL};
        mpv_command_async(mpv, 0, args);
    }
    else
        emit ErrorSignal("mpv was not initialized");
}

void MpvHandler::PlayPause(bool justPause)
{
    if(mpv)
    {
        if(justPause)
        {
            const char *args[] = {"set", "pause", "yes", NULL};
            mpv_command_async(mpv, 0, args);
        }
        else
        {
            const char *args[] = {"cycle", "pause", NULL};
            mpv_command_async(mpv, 0, args);
        }
    }
    else
        emit ErrorSignal("mpv was not initialized");
}

void MpvHandler::Stop()
{
    if(mpv)
    {
        Seek(0);
        PlayPause(true);
    }
    else
        emit ErrorSignal("mpv was not initialized");
}

void MpvHandler::Rewind()
{
    if(mpv)
    {
        const char *args[] = {"seek", "0", "absolute", NULL};
        mpv_command_async(mpv, 0, args);
    }
    else
        emit ErrorSignal("mpv was not initialized");
}

void MpvHandler::Seek(int pos, bool relative)
{
    if(mpv)
    {
        const char *args[] = {"seek",
                              QString::number(pos).toUtf8().data(),
                              relative ? "relative" : "absolute",
                              NULL};
        mpv_command_async(mpv, 0, args);
    }
    else
        emit ErrorSignal("mpv was not initialized");
}

void MpvHandler::AdjustVolume(int level)
{
    if(mpv)
    {
        const char *args[] = {"set", "volume",
                              QString::number(level).toUtf8().data(),
                              NULL};
        mpv_command_async(mpv, 0, args);
    }
    else
        emit ErrorSignal("mpv was not initialized");
}

void MpvHandler::SetFile(QString f)
{
    if(file != f)
    {
        file = f;
        emit FileChanged(f);
    }
}

void MpvHandler::SetTime(time_t t)
{
    if(time != t)
    {
        time = t;
        emit TimeChanged(t);
    }
}

void MpvHandler::SetTotalTime(time_t t)
{
    if(totalTime != t)
    {
        totalTime = t;
        emit TotalTimeChanged(t);
    }
}

void MpvHandler::SetVolume(int v)
{
    if(volume != v)
    {
        volume = v;
        emit VolumeChanged(v);
    }
}

void MpvHandler::SetPlayState(Mpv::PlayState s)
{
    if (playState != s)
    {
        playState = s;
        emit PlayStateChanged(s);
    }
}
