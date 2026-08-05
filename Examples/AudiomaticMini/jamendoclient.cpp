#include "jamendoclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrlQuery>

namespace {

constexpr char kJamendoApiRoot[] = "https://api.jamendo.com/v3.0/";
constexpr char kSettingsGroup[] = "audiomatic";
constexpr char kSettingsClientIdKey[] = "jamendoClientId";
constexpr char kSettingsOrg[] = "Window11Style";
constexpr char kSettingsApp[] = "AudiomaticMini";

QSettings makeJamendoSettings()
{
    return QSettings(
        QSettings::NativeFormat,
        QSettings::UserScope,
        QString::fromLatin1(kSettingsOrg),
        QString::fromLatin1(kSettingsApp));
}

QUrl jamendoUrl(const QString &path, const QUrlQuery &query)
{
    QUrl url(QString::fromLatin1(kJamendoApiRoot) + path);
    url.setQuery(query);
    return url;
}

QUrlQuery baseQuery(const QString &clientId)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("client_id"), clientId);
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    return query;
}

QString validateJamendoRoot(const QJsonObject &root)
{
    const QJsonObject headers = root.value(QStringLiteral("headers")).toObject();
    const QString status = headers.value(QStringLiteral("status")).toString();
    if (status == QStringLiteral("success"))
    {
        return {};
    }

    const QString code = headers.value(QStringLiteral("code")).toString();
    const QString message = headers.value(QStringLiteral("error_message")).toString();
    if (code == QStringLiteral("missing_parameter")
        || message.contains(QStringLiteral("client_id"), Qt::CaseInsensitive))
    {
        return JamendoClient::tr("Jamendo Client ID 无效或未授权。");
    }
    return message.isEmpty() ? JamendoClient::tr("Jamendo API 返回错误。") : message;
}

QList<JamendoTrack> parseTracks(const QJsonArray &results)
{
    QList<JamendoTrack> tracks;
    tracks.reserve(results.size());

    for (const QJsonValue &value : results)
    {
        const QJsonObject item = value.toObject();
        const QUrl audioUrl(item.value(QStringLiteral("audio")).toString());
        if (!audioUrl.isValid() || audioUrl.isEmpty())
        {
            continue;
        }

        JamendoTrack track;
        track.id = item.value(QStringLiteral("id")).toString();
        track.name = item.value(QStringLiteral("name")).toString();
        track.artistName = item.value(QStringLiteral("artist_name")).toString();
        track.durationSec = item.value(QStringLiteral("duration")).toInt();
        track.audioUrl = audioUrl;
        track.imageUrl = QUrl(item.value(QStringLiteral("image")).toString());
        tracks.append(track);
    }

    return tracks;
}

QNetworkRequest buildRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("AudiomaticMini/1.0"));
    return request;
}

} // namespace

QString JamendoTrack::displayTitle() const
{
    if (artistName.isEmpty())
    {
        return name;
    }
    return QStringLiteral("%1 - %2").arg(name, artistName);
}

JamendoClient::JamendoClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    m_clientId = loadJamendoClientId();
}

JamendoClient::~JamendoClient()
{
    cancelActiveRequest();
}

void JamendoClient::setClientId(const QString &clientId)
{
    m_clientId = clientId.trimmed();
    saveJamendoClientId(m_clientId);
}

QString JamendoClient::clientId() const
{
    return m_clientId;
}

bool JamendoClient::hasClientId() const
{
    return !m_clientId.isEmpty();
}

bool JamendoClient::ensureClientId(JamendoRequestKind kind)
{
    if (hasClientId())
    {
        return true;
    }

    emit requestFailed(
        kind,
        tr("未配置 Jamendo Client ID。\n请在设置中填写，或设置环境变量 JAMENDO_CLIENT_ID。\n免费注册：https://devportal.jamendo.com"));
    return false;
}

void JamendoClient::cancelActiveRequest()
{
    if (!m_activeReply)
    {
        return;
    }

    m_activeReply->disconnect(this);
    m_activeReply->abort();
    m_activeReply->deleteLater();
    m_activeReply = nullptr;
}

void JamendoClient::beginGet(const QUrl &url, JamendoRequestKind kind)
{
    cancelActiveRequest();
    m_activeKind = kind;
    emit requestStarted(kind);
    m_activeReply = m_nam->get(buildRequest(url));
    connect(m_activeReply, &QNetworkReply::finished, this, &JamendoClient::onReplyFinished);
}

void JamendoClient::searchTracks(const QString &query, int limit)
{
    if (!ensureClientId(JamendoRequestKind::Search))
    {
        return;
    }

    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty())
    {
        emit requestFailed(JamendoRequestKind::Search, tr("请输入搜索关键词。"));
        return;
    }

    QUrlQuery queryParams = baseQuery(m_clientId);
    queryParams.addQueryItem(QStringLiteral("limit"), QString::number(qBound(1, limit, 50)));
    queryParams.addQueryItem(QStringLiteral("search"), trimmed);
    queryParams.addQueryItem(QStringLiteral("audioformat"), QStringLiteral("mp32"));
    queryParams.addQueryItem(QStringLiteral("include"), QStringLiteral("musicinfo"));

    beginGet(jamendoUrl(QStringLiteral("tracks/"), queryParams), JamendoRequestKind::Search);
}

void JamendoClient::fetchWeeklyTop(int limit)
{
    if (!ensureClientId(JamendoRequestKind::WeeklyTop))
    {
        return;
    }

    QUrlQuery queryParams = baseQuery(m_clientId);
    queryParams.addQueryItem(QStringLiteral("limit"), QString::number(qBound(1, limit, 50)));
    queryParams.addQueryItem(QStringLiteral("order"), QStringLiteral("popularity_week"));
    queryParams.addQueryItem(QStringLiteral("audioformat"), QStringLiteral("mp32"));
    queryParams.addQueryItem(QStringLiteral("include"), QStringLiteral("musicinfo"));

    beginGet(jamendoUrl(QStringLiteral("tracks/"), queryParams), JamendoRequestKind::WeeklyTop);
}

void JamendoClient::fetchEditorialTracks(int limit)
{
    if (!ensureClientId(JamendoRequestKind::Editorial))
    {
        return;
    }

    QUrlQuery queryParams = baseQuery(m_clientId);
    queryParams.addQueryItem(QStringLiteral("limit"), QString::number(qBound(1, limit, 50)));

    beginGet(jamendoUrl(QStringLiteral("feeds/"), queryParams), JamendoRequestKind::Editorial);
}

void JamendoClient::fetchTracksByIds(const QStringList &ids, JamendoRequestKind resultKind, int limit)
{
    if (ids.isEmpty())
    {
        emit requestFailed(resultKind, tr("编辑精选暂无曲目。"));
        return;
    }

    QStringList uniqueIds;
    uniqueIds.reserve(ids.size());
    for (const QString &id : ids)
    {
        const QString trimmed = id.trimmed();
        if (!trimmed.isEmpty() && !uniqueIds.contains(trimmed))
        {
            uniqueIds.append(trimmed);
        }
        if (uniqueIds.size() >= limit)
        {
            break;
        }
    }

    QUrlQuery queryParams = baseQuery(m_clientId);
    queryParams.addQueryItem(QStringLiteral("id"), uniqueIds.join(QLatin1Char('+')));
    queryParams.addQueryItem(QStringLiteral("audioformat"), QStringLiteral("mp32"));
    queryParams.addQueryItem(QStringLiteral("include"), QStringLiteral("musicinfo"));

    beginGet(jamendoUrl(QStringLiteral("tracks/"), queryParams), resultKind);
}

void JamendoClient::fetchRadios()
{
    if (!ensureClientId(JamendoRequestKind::Radios))
    {
        return;
    }

    beginGet(jamendoUrl(QStringLiteral("radios/"), baseQuery(m_clientId)), JamendoRequestKind::Radios);
}

void JamendoClient::fetchRadioStream(int radioId)
{
    if (!ensureClientId(JamendoRequestKind::RadioStream))
    {
        return;
    }

    if (radioId <= 0)
    {
        emit requestFailed(JamendoRequestKind::RadioStream, tr("无效的电台。"));
        return;
    }

    QUrlQuery queryParams = baseQuery(m_clientId);
    queryParams.addQueryItem(QStringLiteral("id"), QString::number(radioId));

    beginGet(jamendoUrl(QStringLiteral("radios/stream/"), queryParams), JamendoRequestKind::RadioStream);
}

void JamendoClient::onReplyFinished()
{
    if (!m_activeReply)
    {
        return;
    }

    QNetworkReply *reply = m_activeReply;
    const JamendoRequestKind kind = m_activeKind;
    m_activeReply = nullptr;

    if (reply->error() != QNetworkReply::NoError)
    {
        const QString message = tr("网络请求失败：%1").arg(reply->errorString());
        reply->deleteLater();
        emit requestFailed(kind, message);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    reply->deleteLater();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        emit requestFailed(kind, tr("无法解析 Jamendo 响应。"));
        return;
    }

    const QJsonObject root = doc.object();
    const QString apiError = validateJamendoRoot(root);
    if (!apiError.isEmpty())
    {
        emit requestFailed(kind, apiError);
        return;
    }

    const QJsonArray results = root.value(QStringLiteral("results")).toArray();

    if (kind == JamendoRequestKind::Editorial)
    {
        QStringList trackIds;
        trackIds.reserve(results.size());
        for (const QJsonValue &value : results)
        {
            const QJsonObject item = value.toObject();
            if (item.value(QStringLiteral("type")).toString() != QStringLiteral("track"))
            {
                continue;
            }

            const QString trackId = item.value(QStringLiteral("joinid")).toString();
            if (!trackId.isEmpty())
            {
                trackIds.append(trackId);
            }
        }

        fetchTracksByIds(trackIds, JamendoRequestKind::Editorial, 20);
        return;
    }

    if (kind == JamendoRequestKind::Radios)
    {
        QList<JamendoRadio> radios;
        radios.reserve(results.size());
        for (const QJsonValue &value : results)
        {
            const QJsonObject item = value.toObject();
            JamendoRadio radio;
            radio.id = item.value(QStringLiteral("id")).toInt();
            radio.name = item.value(QStringLiteral("name")).toString();
            radio.displayName = item.value(QStringLiteral("dispname")).toString();
            if (radio.displayName.isEmpty())
            {
                radio.displayName = radio.name;
            }
            radio.imageUrl = QUrl(item.value(QStringLiteral("image")).toString());
            if (radio.id > 0 && !radio.displayName.isEmpty())
            {
                radios.append(radio);
            }
        }

        if (radios.isEmpty())
        {
            emit requestFailed(kind, tr("未找到电台。"));
            return;
        }

        emit radiosReady(radios);
        return;
    }

    if (kind == JamendoRequestKind::RadioStream)
    {
        if (results.isEmpty())
        {
            emit requestFailed(kind, tr("无法获取电台流地址。"));
            return;
        }

        const QJsonObject item = results.first().toObject();
        JamendoRadio radio;
        radio.id = item.value(QStringLiteral("id")).toInt();
        radio.name = item.value(QStringLiteral("name")).toString();
        radio.displayName = item.value(QStringLiteral("dispname")).toString();
        if (radio.displayName.isEmpty())
        {
            radio.displayName = radio.name;
        }
        radio.imageUrl = QUrl(item.value(QStringLiteral("image")).toString());
        radio.streamUrl = QUrl(item.value(QStringLiteral("stream")).toString());

        if (!radio.streamUrl.isValid() || radio.streamUrl.isEmpty())
        {
            emit requestFailed(kind, tr("电台流地址无效。"));
            return;
        }

        emit radioStreamReady(radio);
        return;
    }

    const QList<JamendoTrack> tracks = parseTracks(results);
    if (tracks.isEmpty())
    {
        emit requestFailed(kind, tr("未找到可播放的结果。"));
        return;
    }

    emit tracksReady(kind, tracks);
}

QString loadJamendoClientId()
{
    const QByteArray env = qgetenv("JAMENDO_CLIENT_ID");
    if (!env.isEmpty())
    {
        return QString::fromUtf8(env);
    }

#ifdef JAMENDO_CLIENT_ID
    const QString compiled = QStringLiteral(JAMENDO_CLIENT_ID);
    if (!compiled.isEmpty())
    {
        return compiled;
    }
#endif

    QSettings settings = makeJamendoSettings();
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    const QString stored = settings.value(QString::fromLatin1(kSettingsClientIdKey)).toString();
    settings.endGroup();
    return stored;
}

void saveJamendoClientId(const QString &clientId)
{
    QSettings settings = makeJamendoSettings();
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    settings.setValue(QString::fromLatin1(kSettingsClientIdKey), clientId.trimmed());
    settings.endGroup();
    settings.sync();
}
