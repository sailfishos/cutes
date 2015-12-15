/*
 * QtScript Actor implementation
 *
 * Copyright (C) 2012 Jolla Ltd.
 * Contact: Denis Zalevskiy <denis.zalevskiy@jollamobile.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include "Actor.hpp"
#include "ActorEvents.hpp"
#include "Env.hpp"
#include "QmlAdapter.hpp"

#include "qt_quick_types.hpp"

//#include <cutes/util.hpp>
#include <cor/util.hpp>

#include <QDebug>
#include <QCoreApplication>
#include <QMap>
#include <QDateTime>
#include <QJSValueIterator>
#include <mutex>

namespace cutes {

#define MAX_MSG_DEPTH 50

QMutex Actor::actors_mutex_;
std::set<Actor*> Actor::actors_;

void Actor::quitAll()
{
    std::lock_guard<QMutex> lock(actors_mutex_);
    for (auto *actor : actors_) {
        if (isTrace()) tracer() << "Quit actor " << actor;
        actor->worker_->quit();
        actor->worker_->wait();
        if (isTrace()) tracer() << "Quited actor " << actor;
    }
}


static QVariant msgFromValue
(QJSValue const &v
 , JSValueConvert convert
 , JSValueConvertOptions options
 , size_t depth)
{
    if (depth > MAX_MSG_DEPTH)
        throw Error("Reached max msg depth encoding QJSValue. Is there a cycle?");

    // TODO function does not handle cycles
    if (v.isUndefined() || v.isNull()) {
        // do nothing
    } else if (v.isString() || v.isRegExp()) {
        return v.toString();
    } else if (v.isBool()) {
        return v.toBool();
    } else if (v.isDate()) {
        return v.toDateTime();
    } else if (v.isNumber()) {
        return v.toNumber();
    } else if (v.isVariant()) {
        return v.toVariant();
    } else if (v.isArray()) {
        QVariantList res;
        auto len = v.property("length").toUInt();
        for (decltype(len) i = 0; i < len; ++i) {
            auto value = msgFromValue(v.property(i), convert, options, depth + 1);
            res.push_back(value);
        }
        return res;
    } else if (v.isError() && options == JSValueConvertOptions::NoError) {
        throw Error(QString("Can't convert error") + v.toString());
    } else if (v.isObject()) {
        QVariantMap res;
        QJSValueIterator it(v);
        while (it.hasNext()) {
            it.next();
            if (convert == JSValueConvert::Shallow
                && !v.hasOwnProperty(it.name()))
                continue;

            auto value = msgFromValue(it.value(), convert, options, depth + 1);
            if (value.isValid())
                res[it.name()] = value;
        }
        return res;
    }
    return QVariant();
}

QVariant msgFromValue
(QJSValue const &v, JSValueConvert convert, JSValueConvertOptions options)
{
    auto res = msgFromValue(v, convert, options, 0);
    return res;
}

static endpoint_ptr endpoint(QJSValue const& ep
                             , std::unique_ptr<ActorHolder> holder)
{
    QJSValue on_reply, on_error, on_progress;
    if (ep.isCallable()) {
        on_reply = ep;
    } else if (ep.isObject()) {
        on_reply = ep.property("on_reply");
        if (!on_reply.isCallable())
            on_reply = ep.property("on_done");
        on_error = ep.property("on_error");
        on_progress = ep.property("on_progress");
        if (!(on_reply.isCallable() || on_progress.isCallable()))
            if (isTrace())
                tracer() << "on_reply or on_progress are not callable";
    } else if (!ep.isNull()) {
        throw Error(QString("Wrong endpoint? Expecting object, function or null")
                    + ep.toString());
    }
    return endpoint_ptr(new Endpoint(on_reply, on_error, on_progress
                                     , std::move(holder)));
}

Actor::Actor(QJSEngine *engine)
    : QObject(engine)
    , engine_(engine)
    , unreplied_count_(0)
    , cookie_(0)
{
    if (isTrace()) tracer() << " New actor" << this;
    std::lock_guard<QMutex> lock(actors_mutex_);
    actors_.insert(this);
}

Actor::~Actor()
{
    std::lock_guard<QMutex> lock(actors_mutex_);
    actors_.erase(this);
}

QString Actor::source() const
{
    return src_;
}

void Actor::execute(std::function<void ()> fn)
{
    try{
        fn();
    } catch (Error const &e) {
        qWarning() << "Error executing request/send:" << e.msg;
        // TODO throw js error
    } catch (...) {
        qWarning() << "Some error executing request/send";
        // TODO throw js error
    }
}

template <typename T>
static inline T* contextProperty(QJSEngine &engine, QString const& name)
{
    auto qmleng = dynamic_cast<QQmlEngine*>(&engine);
    QVariant v = (!qmleng
                  ? engine.globalObject().property(name).toVariant()
                  : qmleng->rootContext()->contextProperty(name));
    return v.value<T*>();
}

void Actor::reload()
{
    auto fn = [this]() {
        if (!engine_) {
            qWarning() << "Can't get script engine."
            << " Not qml engine and missing initialization?";
            return;
        }
        auto env = contextProperty<EnvImpl>(*engine_, "cutes");
        if (!env) {
            qWarning() << "No env set in context?";
            return;
        }
        // cwd should be set to the same directory as for main engine
        auto script = env->currentModule();

        worker_.reset(new WorkerThread(this, src_, script->fileName(), cor::make_unique<ActorHolder>(this)));
    };
    execute(fn);
}

void Actor::setSource(QString const& src)
{
    if (isTrace()) tracer() << "Actor src:" << src;
    if (src == src_)
        return;

    src_ = src;
    reload();
}

QmlActor::QmlActor(QJSEngine *engine)
    : Actor(engine)
{
}

QUrl QmlActor::source() const
{
    return Actor::source();
}

void QmlActor::setSource(QUrl const& src)
{
    // if Actor is created by QtScript environment it should have
    // engine_ set while declarative view does not provide direct
    // access to the engine and it will be initialized on first call
    // to this method
    if (!engine_)
        engine_ = qmlEngine(this);

    if (!engine_) {
        qWarning() << "CutesActor have not engine assigned. No go...";
        return;
    }
    Actor::setSource(src.path());
}

StdActor::StdActor(QJSEngine *engine)
    : Actor(engine)
{}

// QUrl StdActor::source() const
// {
//     return Actor::source();
// }

// void StdActor::setSource(QUrl const& src)
// {
//     Actor::setSource(src.path());
// }

WorkerThread *Actor::worker()
{
    if (!worker_)
        throw Error("Non-initialized worker");
    return worker_.data();
}

void WorkerThread::send(Message *msg)
{
    QCoreApplication::postEvent(engine_.data(), msg);
}

void Actor::acquire()
{
    if (!unreplied_count_)
        emit acquired();

    ++unreplied_count_;
}

void Actor::release()
{
    if (--unreplied_count_ == 0)
        emit released();
}

endpoint_handle Actor::endpoint_new(QJSValue cb)
{
    auto ep = endpoint(cb, cor::make_unique<ActorHolder>(this));
    endpoints_.insert(ep.data(), ep);
    endpoint_handle h(ep.data(), [this](Endpoint *p) {
            QCoreApplication::postEvent(this, new EndpointRemove(p));
        });
    return h;
}

void Actor::send(QJSValue const &msg, QJSValue cb)
{
    auto fn = [&]() {
        worker()->send
        (new Message(msgFromValue(msg), endpoint_new(cb), Event::Message));
    };
    execute(fn);
}

void Actor::request(QString const &method, QJSValue msg, QJSValue cb)
{
    auto fn = [&]() {
        worker_->send(new Request(method, msgFromValue(msg)
                                  , endpoint_new(cb), Event::Request));
    };
    execute(fn);
}

void Actor::reply(Message *msg)
{
    callback(msg, msg->endpoint_->on_reply_);
}
void Actor::progress(Message *msg)
{
    callback(msg, msg->endpoint_->on_progress_);
}

void Actor::callback(Message *msg, QJSValue& cb)
{
    if (!cb.isCallable())
        return;
    auto params = QJSValueList();
    params.push_back(engine_->toScriptValue(msg->data_));
    cb.callWithInstance(cb, params);
}

void Actor::error(Message *reply)
{
    auto &cb = reply->endpoint_->on_error_;
    if (!cb.isCallable()) {
        qWarning() << "Error: " << reply->data_;
        emit error(reply->data_);
        return;
    }
    if (isTrace()) {
        tracer() << "Processing actor error"
                << reply->data_;
        tracer() << " Error processing function:" << cb.toString();
    }
    QJSValueList params;
    if (engine_) {
        params.push_back(engine_->toScriptValue(reply->data_));
    } else {
        qWarning() << "!engine_";
    }
    cb.callWithInstance(cb, params);
}

Event::Event()
    : QEvent(static_cast<QEvent::Type>(Event::QuitThread))
{
}

Event::Event(Event::Type t)
    : QEvent(static_cast<QEvent::Type>(t))
{ }

Event::~Event() {}

WorkerThread::WorkerThread
(Actor *actor, QString const &src, QString const& top_script
 , std::unique_ptr<ActorHolder> holder)
    : QThread(nullptr)
    , actor_(actor)
{
    std::lock_guard<QMutex> lock(mutex_);
    start();
    cond_.wait(&mutex_);
    QCoreApplication::postEvent(engine_.data(), new Load(src, top_script, std::move(holder)));
    connect(engine_.data(), SIGNAL(onQuit()), this, SLOT(quit()));
}

void Engine::toActor(Event *ev)
{
    if (isTrace()) tracer() << "toActor" << actor_ << ": " << ev;
    QCoreApplication::postEvent(actor_, ev);
}

void Engine::load(Load *msg)
{
    if (!msg)
        throw Error("Logical error: !msg");

    if (isTrace()) tracer() << "Loading actor code from " << msg->src_;
    engine_.reset(new QJSEngine(this));
    try {
        auto script_env = new EnvImpl
            (engine_.data(), *QCoreApplication::instance(), *engine_);
        script_env->pushParentScriptPath(msg->top_script_);
        handler_ = script_env->load(msg->src_, false);
        if (isTrace()) tracer() << msg->src_ << ": loaded " << handler_.toString();
        if (handler_.isError()) {
            qWarning() << "Error while loading " << msg->src_
                       << ":" << handler_.toString();
            auto info = msgFromValue(handler_, JSValueConvert::Deep
                                     , JSValueConvertOptions::AllowError);
            toActor(new LoadError(info, std::move(msg->actor_holder_)));
        } else if (!(handler_.isCallable() || handler_.isObject())) {
            qWarning() << msg->src_ << ": not a function or object but "
                       << handler_.toString();
        } else {
            toActor(new Load(std::move(*msg)));
        }
    } catch (Error const &e) {
        qWarning() << "Failed to eval:" << msg->src_;
        qWarning() << e.msg;
        toActor(new LoadError(e.msg, std::move(msg->actor_holder_)));
    }
}

Engine::Engine(Actor *actor) : actor_(actor) {}
Engine::~Engine() {}

WorkerThread::~WorkerThread()
{
    quit();
    wait();
}

void WorkerThread::run()
{
    engine_.reset(new Engine(actor_));
    auto on_exit = cor::on_scope_exit([this]() {
            engine_.reset();
        });
    std::unique_lock<QMutex> lock(mutex_);
    cond_.wakeAll();
    lock.unlock();
    exec();
}

void Engine::processResult(QJSValue ret, endpoint_handle ep)
{
    if (isTrace()) tracer() << "Actor result:" << ret.toString();
    try {
        if (!ret.isError()) {
            if (isTrace())
                tracer() << "Actor returned" << ret.toString();
            reply(msgFromValue(ret), ep, Event::Return);
        } else {
            QVariantMap err;
            for (auto p : {"message", "stack"
                        //, "arguments" TODO depends on cycles processing
                        // in msgFromValue()
                        , "type", "isWrapped", "originalError", "lineNumber"
                        , "name", "fileName", "columnNumber"
                        , "reason"}) {
                auto v = msgFromValue(ret.property(p), JSValueConvert::Shallow);
                if (v.isValid())
                    err[p] = v;
            }

            qWarning() << "Actor returned error:" << err;
            error(err, ep);
        }
    } catch (Error const &e) {
        qWarning() << "Error processing result:" << e.msg;
        error(QVariant(e.msg), ep);
    } catch (std::exception const& e) {
        qWarning() << "Error processing result:" << e.what();
        error(QVariant(e.what()), ep);
    } catch(...) {
        qWarning() << "Error processing result:";
        error(QVariant(), ep);
    }
}

QJSValue Engine::callConvertError(QJSValue const &fn
                                  , QJSValue const &instance
                                  , QJSValueList const &params)
{
    if (isTrace()) tracer() << "Call: " << fn.toString();
    if (convert_error_.isUndefined()) {
        QString prelude =
            "var cutes = Object.cutes__;"
            "var res = function(fn, obj) {";
        auto code = errorConverterTry(prelude)
            + "return fn.apply(obj, [].slice.call(arguments, 2));"
            + errorConverterCatch("}; res;");
        if (!engine_) {
            qWarning() << "Engine is null!";
            return convert_error_;
        }
        convert_error_ = engine_->evaluate(code);
        if (!convert_error_.isCallable()) {
            qWarning() << "Can't evaluate properly error conversion code: "
                       << "expected conversion function, got "
                       << convert_error_.toString();
            convert_error_ = QJSValue();
            return convert_error_;
        }
    }

    QJSValueList converter_params(params);
    converter_params.push_front(instance);
    converter_params.push_front(fn);
    return convert_error_.call(converter_params);
}

void Engine::processMessage(Message *msg)
{
    QJSValue ret;
    auto reply_ep = msg->endpoint_;
    auto on_exit = cor::on_scope_exit([this, reply_ep, &ret]() noexcept {
            processResult(ret, reply_ep);
        });

    if (handler_.isCallable()) {
        auto ctx = MessageContext::create(this, reply_ep);
        QJSValueList params;
        params.push_back(engine_->toScriptValue(msg->data_));
        params.push_back(ctx.jsObject(*engine_));
        ret = callConvertError(handler_, handler_, params);
    } else if (!(handler_.isNull() && handler_.isUndefined())) {
        qWarning() << "Handler is not a function but "
                 << (handler_.toString());
    } else {
        qWarning() << "No handler";
    }
}

void Engine::processRequest(Request *req)
{
    QJSValue ret;
    auto reply_ep = req->endpoint_;
    auto on_exit = cor::on_scope_exit([this, reply_ep, &ret]() noexcept {
            processResult(ret, reply_ep);
        });

    if (!handler_.isObject()) {
        qWarning() << "Handler is not an object";
    }

    auto method = handler_.property(req->method_name_);
    if (method.isCallable()) {
        auto ctx = MessageContext::create(this, reply_ep);
        QJSValueList params;
        params.push_back(engine_->toScriptValue(req->data_));
        params.push_back(ctx.jsObject(*engine_));
        ret = callConvertError(method, handler_, params);
    } else if (method.isUndefined() || method.isNull()) {
        qWarning() << "Actor does not have method" << req->method_name_;
    } else {
        qWarning() << "Actor property " << req->method_name_
                   << " is not a method but "
                   << method.toString();
    }
}

bool Engine::event(QEvent *e)
{
    try {
        switch (static_cast<Event::Type>(e->type())) {
        case (Event::LoadScript):
            load(static_cast<Load*>(e));
            return true;
        case (Event::Message):
            processMessage(static_cast<Message*>(e));
            return true;
        case (Event::Request):
            processRequest(static_cast<Request*>(e));
            return true;
        case (Event::QuitThread):
            emit onQuit();
            return true;
        default:
            return QObject::event(e);
        }
    } catch (Error const &e) {
        qWarning() << "Error processing event:" << e.msg;
        // TODO pass error
    } catch (std::exception const& e) {
        qWarning() << "Error processing event:" << e.what();
        // TODO pass error
    } catch (...) {
        qWarning() << "Some error processing event";
        // TODO pass error
    }
    return false;
}

bool Actor::event(QEvent *e)
{
    LoadError *ex;
    bool res;
    switch (static_cast<Event::Type>(e->type())) {
    case (Event::Progress):
        progress(static_cast<Message*>(e));
        res = true;
        break;
    case (Event::Return):
        reply(static_cast<Message*>(e));
        res = true;
        break;
    case (Event::LoadException):
        if (isTrace()) tracer() << "LoadException";
        ex = static_cast<LoadError*>(e);
        emit error(ex->info_);
        res = true;
        break;
    case (Event::Error):
        error(static_cast<Message*>(e));
        res = true;
        break;
    case Event::EndpointRemove: {
        EndpointRemove *rm = static_cast<EndpointRemove*>(e);
        endpoints_.remove(rm->endpoint_);
        res = true;
        break;
    }
    case (Event::LoadScript):
        emit loaded();
        return true;
    default:
        res = QObject::event(e);
        break;
    }
    return res;
}

MessageContext::MessageContext(Engine *engine, endpoint_handle ep)
    : QObject(engine)
    , engine_(engine)
    , endpoint_(ep)
{}
MessageContext::~MessageContext() {}

void MessageContext::disable()
{
    endpoint_.reset();
}

void Actor::wait()
{
    if (isTrace()) tracer() << "wait";
    if (!unreplied_count_) {
        if (isTrace()) tracer() << "returned already";
        return;
    }

    QEventLoop loop;
    QObject::connect(this, SIGNAL(released()), &loop, SLOT(quit()));
    loop.exec();
    if (isTrace()) tracer() << "actor replied";
}

void MessageContext::reply(QJSValue data)
{
    if (engine_)
        engine_->reply(msgFromValue(data), endpoint_, Event::Progress);
    else
        qWarning() << "MessageContext is disabled, " <<
            "are you using it outside processing function?";
}

void Engine::reply
(QVariant const &data, endpoint_handle ep, Event::Type type)
{
    toActor(new Message(data, ep, type));
}

void Engine::error(QVariant const &data, endpoint_handle ep)
{
    toActor(new Message(data, ep, Event::Error));
}


}
