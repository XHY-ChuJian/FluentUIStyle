#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

struct JamendoTrack
{
    QString id;
    QString name;
    QString artistName;
    int durationSec{0};
    QUrl audioUrl;
    QUrl imageUrl;

    QString displayTitle() const;
};

struct JamendoRadio
{
    int id{0};
    QString name;
    QString displayName;
    QUrl imageUrl;
    QUrl streamUrl;
};

enum class JamendoRequestKind
{
    Search,
    WeeklyTop,
    Editorial,
    Radios,
    RadioStream
};

/**
 * @brief Jamendo API v3.0 客户端（搜索、热榜、编辑精选、电台）。
 *
 * 需在 https://devportal.jamendo.com 注册应用获取 client_id。
 */
class JamendoClient : public QObject
{
    Q_OBJECT

public:
    explicit JamendoClient(QObject *parent = nullptr);
    ~JamendoClient() override;

    void setClientId(const QString &clientId);
    QString clientId() const;
    bool hasClientId() const;

    void searchTracks(const QString &query, int limit = 20);
    void fetchWeeklyTop(int limit = 20);
    void fetchEditorialTracks(int limit = 20);
    void fetchRadios();
    void fetchRadioStream(int radioId);

signals:
    void requestStarted(JamendoRequestKind kind);
    void tracksReady(JamendoRequestKind kind, const QList<JamendoTrack> &tracks);
    void radiosReady(const QList<JamendoRadio> &radios);
    void radioStreamReady(const JamendoRadio &radio);
    void requestFailed(JamendoRequestKind kind, const QString &message);

private:
    void onReplyFinished();
    void cancelActiveRequest();
    bool ensureClientId(JamendoRequestKind kind);
    void beginGet(const QUrl &url, JamendoRequestKind kind);
    void fetchTracksByIds(const QStringList &ids, JamendoRequestKind resultKind, int limit);

    QNetworkAccessManager *m_nam{nullptr};
    QNetworkReply *m_activeReply{nullptr};
    JamendoRequestKind m_activeKind{JamendoRequestKind::Search};
    QString m_clientId;
};

QString loadJamendoClientId();
void saveJamendoClientId(const QString &clientId);
