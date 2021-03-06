#include <QtCore/QPair>
#include <QtCore/QSocketNotifier>
#include <QtCore/QThread>
#include "eventdispatcher_libev.h"
#include "eventdispatcher_libev_p.h"

EventDispatcherLibEv::EventDispatcherLibEv(QObject* parent)
	: QAbstractEventDispatcher(parent), d_ptr(new EventDispatcherLibEvPrivate(this))
{
}

EventDispatcherLibEv::~EventDispatcherLibEv(void)
{
#if QT_VERSION < 0x040600
	delete this->d_ptr;
	this->d_ptr = 0;
#endif
}

bool EventDispatcherLibEv::processEvents(QEventLoop::ProcessEventsFlags flags)
{
	Q_D(EventDispatcherLibEv);
	return d->processEvents(flags);
}

bool EventDispatcherLibEv::hasPendingEvents(void)
{
	extern uint qGlobalPostedEventsCount();
	return qGlobalPostedEventsCount() > 0;
}

void EventDispatcherLibEv::registerSocketNotifier(QSocketNotifier* notifier)
{
#ifndef QT_NO_DEBUG
	if (notifier->socket() < 0) {
		qWarning("QSocketNotifier: Internal error: sockfd < 0");
		return;
	}

	if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
		qWarning("QSocketNotifier: socket notifiers cannot be enabled from another thread");
		return;
	}
#endif

	if (notifier->type() == QSocketNotifier::Exception) {
		return;
	}

	Q_D(EventDispatcherLibEv);
	d->registerSocketNotifier(notifier);
}

void EventDispatcherLibEv::unregisterSocketNotifier(QSocketNotifier* notifier)
{
#ifndef QT_NO_DEBUG
	if (notifier->socket() < 0) {
		qWarning("QSocketNotifier: Internal error: sockfd < 0");
		return;
	}

	if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
		qWarning("QSocketNotifier: socket notifiers cannot be disabled from another thread");
		return;
	}
#endif

	// Short circuit, we do not support QSocketNotifier::Exception
	if (notifier->type() == QSocketNotifier::Exception) {
		return;
	}

	Q_D(EventDispatcherLibEv);
	d->unregisterSocketNotifier(notifier);
}

void EventDispatcherLibEv::registerTimer(
	int timerId,
	int interval,
#if QT_VERSION >= 0x050000
	Qt::TimerType timerType,
#endif
	QObject* object
)
{
#ifndef QT_NO_DEBUG
	if (timerId < 1 || interval < 0 || !object) {
		qWarning("%s: invalid arguments", Q_FUNC_INFO);
		return;
	}

	if (object->thread() != this->thread() && this->thread() != QThread::currentThread()) {
		qWarning("%s: timers cannot be started from another thread", Q_FUNC_INFO);
		return;
	}
#endif

	Qt::TimerType type;
#if QT_VERSION >= 0x050000
	type = timerType;
#else
	type = Qt::CoarseTimer;
#endif

	Q_D(EventDispatcherLibEv);
	d->registerTimer(timerId, interval, type, object);
}

bool EventDispatcherLibEv::unregisterTimer(int timerId)
{
#ifndef QT_NO_DEBUG
	if (timerId < 1) {
		qWarning("%s: invalid arguments", Q_FUNC_INFO);
		return false;
	}

	if (this->thread() != QThread::currentThread()) {
		qWarning("%s: timers cannot be stopped from another thread", Q_FUNC_INFO);
		return false;
	}
#endif

	Q_D(EventDispatcherLibEv);
	return d->unregisterTimer(timerId);
}

bool EventDispatcherLibEv::unregisterTimers(QObject* object)
{
#ifndef QT_NO_DEBUG
	if (!object) {
		qWarning("%s: invalid arguments", Q_FUNC_INFO);
		return false;
	}

	if (object->thread() != this->thread() && this->thread() != QThread::currentThread()) {
		qWarning("%s: timers cannot be stopped from another thread", Q_FUNC_INFO);
		return false;
	}
#endif

	Q_D(EventDispatcherLibEv);
	return d->unregisterTimers(object);
}

QList<QAbstractEventDispatcher::TimerInfo> EventDispatcherLibEv::registeredTimers(QObject* object) const
{
	if (!object) {
		qWarning("%s: invalid argument", Q_FUNC_INFO);
		return QList<QAbstractEventDispatcher::TimerInfo>();
	}

	Q_D(const EventDispatcherLibEv);
	return d->registeredTimers(object);
}

#if QT_VERSION >= 0x050000
int EventDispatcherLibEv::remainingTime(int timerId)
{
	Q_D(const EventDispatcherLibEv);
	return d->remainingTime(timerId);
}
#endif

#if defined(Q_OS_WIN) && QT_VERSION >= 0x050000
bool EventDispatcherLibEv::registerEventNotifier(QWinEventNotifier* notifier)
{
	Q_UNUSED(notifier)
	Q_UNIMPLEMENTED();
	return false;
}

void EventDispatcherLibEv::unregisterEventNotifier(QWinEventNotifier* notifier)
{
	Q_UNUSED(notifier)
	Q_UNIMPLEMENTED();
}
#endif

void EventDispatcherLibEv::wakeUp(void)
{
	Q_D(EventDispatcherLibEv);

#if QT_VERSION >= 0x040400
	if (d->m_wakeups.testAndSetAcquire(0, 1))
#endif
	{
		ev_async_send(d->m_base, &d->m_wakeup);
	}
}

void EventDispatcherLibEv::interrupt(void)
{
	Q_D(EventDispatcherLibEv);
	d->m_interrupt = true;
	this->wakeUp();
}

void EventDispatcherLibEv::flush(void)
{
}

EventDispatcherLibEv::EventDispatcherLibEv(EventDispatcherLibEvPrivate& dd, QObject* parent)
	: QAbstractEventDispatcher(parent), d_ptr(&dd)
{
}
