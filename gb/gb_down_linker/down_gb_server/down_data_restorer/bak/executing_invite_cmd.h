#ifndef __EXECUTING_INVITE_CMD_H__
#define __EXECUTING_INVITE_CMD_H__

#include <string>
#include <variant>
#include <memory>
#include <sstream>

namespace GBGateway {

typedef struct ExecutingInviteCmd
{
    enum class CmdType : uint8_t
    {
        LiveStream = 0,
        RecordPlayback = 1,
        RecordDownload = 2,
        VoiceBroadcast = 3,
    };

    typedef struct State
    {
        //实时拉流
        enum class LiveStream : uint8_t
        {
            //没有执行实时拉流操作
            None = 0,
            //已经给设备发送了实时视音频点播消息-Invite
            SendInvite = 1,
            //接收到设备发送过来的Invite-100 Trying回复
            InviteProceeding_Error = 2,
            RecvInviteResponse_100Trying = 3,
            //接收到设备发送过来的Invite-200 OK回复
            RecvInviteResponse_200OK = 4,
            //已经给设备发送了实时视音频点播消息-ACK
            SendACK = 5,
            //已经给设备发送了实时视音频点播消息 - BYE
            SendBYE = 6,
            //接收到设备发送过来的实时视音频点播消息-BYE
            RecvBYE = 7,
        };

        //录像回放
        enum class RecordPlayback : uint8_t
        {
            //没有执行录像回放操作
            None = 0,
            //已经给设备发送了历史视音频回放消息-Invite
            SendInvite = 1,
            //接收到设备发送过来的Invite-100 Trying回复
            RecvInviteResponse_100Trying = 2,
            //接收到设备发送过来的Invite-200 OK回复
            RecvInviteResponse_200OK = 3,
            //已经给设备发送了历史视音频回放消息-ACK
            SendACK = 4,
            //已经给设备发送了历史视音频回放消息-INFO播放控制
            SendPlayControl = 5,
            //接收到设备发送过来的历史视音频回放消息-Message文件发送完成
            RecvEndFileSending = 6,
            //已经给设备发送了历史视音频回放消息-BYE
            SendBYE = 7,
            //接收到设备发送过来的历史视音频回放消息-BYE
            RecvBYE = 8,
        };

        //录像下载
        enum class RecordDownload : uint8_t
        {
            //没有执行录像下载操作
            None = 0,
            //已经给设备发送了历史视音频下载消息-Invite
            SendInvite = 1,
            //接收到设备发送过来的Invite-100 Trying回复
            RecvInviteResponse_100Trying = 2,
            //接收到设备发送过来的Invite-200 OK回
            RecvInviteResponse_200OK = 3,
            //已经给设备发送了历史视音频下载消息-ACK
            SendACK = 4,
            //接收到设备发送过来的历史视音频下载消息-Message文件发送完成
            RecvEndFileSending = 5,
            //已经给设备发送了历史视音频下载消息-BYE
            SendBYE = 6,
            //接收到设备发送过来的历史视音频下载消息-BYE
            RecvBYE = 7,
        };

        //语音广播
        enum class VoiceBroadcast : uint8_t
        {
        };
    } State;

    using InviteState = std::variant<
        State::LiveStream,
        State::RecordDownload,
        State::RecordPlayback,
        State::VoiceBroadcast>;

    typedef struct Info
    {
        typedef struct LiveStream
        {
            //启动实时拉流时的会话ID
            std::string sessionIdStart;
            //媒体流协议
            std::string streamProto;
            //用于接收媒体流的IP地址
            std::string recvStreamIp;
            //用于接收媒体流的Port
            std::string recvStreamPort;
            //IPC等发送端用于发送媒体流的Socket IP
            std::string playIp;
            //IPC等发送端用于发送媒体流的Socket Port
            std::string playPort;
            //IPC等发送端用于发送媒体流的媒体流协议
            std::string playProto;
            //停止实时拉流时的会话ID
            std::string sessionIdStop;
        } LiveStream;

        using LiveStreamPtr = std::unique_ptr<LiveStream>;

        typedef struct RecordPlayback
        {
            enum class PlayAction : uint8_t
            {
                //暂停播放
                Pause = 0,
                //继续播放
                Resume = 1,
                //快速播放
                Fast = 2,
                //慢速播放
                Slow = 3,
                //指定位置播放
                Seek = 4,

            };

            //启动实时拉流时的会话ID
            std::string sessionIdStart;
            //媒体流协议，TCP-TCP/RTP/AVP、UDP-RTP/AVP
            std::string streamProto;
            //用于接收媒体流的IP地址
            std::string recvStreamIp;
            //用于接收媒体流的Port
            std::string recvStreamPort;
            //录像回放开始时间
            std::string playTime;
            //录像回放结束时间
            std::string endTime;
            //IPC等发送端用于发送媒体流的Socket IP
            std::string playIp;
            //IPC等发送端用于发送媒体流的Socket Port
            std::string playPort;
            //IPC等发送端用于发送媒体流的媒体流协议
            std::string playProto;
            //播放控制类型
            PlayAction playAction;
            //播放速度，在“快速播放”和“慢速播放”时有效
            float scale = 0.0f;
            //指定播放时间，在“指定位置播放”时有效
            std::string seekTime;
            //eXosip中的Transaction ID，用于INFO消息（播放控制）
            int sipTidPlay = 0;
            //停止实时拉流时的会话ID
            std::string sessionIdStop;
        } RecordPlayback;

        using RecordPlaybackPtr = std::unique_ptr<RecordPlayback>;

        //录像下载
        typedef struct RecordDownload
        {
            //启动实时拉流时的会话ID
            std::string sessionIdStart;
            //媒体流协议，TCP-TCP/RTP/AVP、UDP-RTP/AVP
            std::string streamProto;
            //用于接收媒体流的IP地址
            std::string recvStreamIp;
            //用于接收媒体流的Port
            std::string recvStreamPort;
            //录像下载开始时间
            std::string playTime;
            //录像下载结束时间
            std::string endTime;
            //录像下载速度
            float speed = 0.0f;
            //IPC等发送端用于发送媒体流的Socket IP
            std::string playIp;
            //IPC等发送端用于发送媒体流的Socket Port
            std::string playPort;
            //IPC等发送端用于发送媒体流的媒体流协议
            std::string playProto;
            //停止实时拉流时的会话ID
            std::string sessionIdStop;
        } RecordDownload;

        using RecordDownloadPtr = std::unique_ptr<RecordDownload>;

        //语音广播
        typedef struct VoiceBroadcast
        {
        } VoiceBroadcast;

        using VoiceBroadcastPtr = std::unique_ptr<VoiceBroadcast>;
    } Info;

    using OperationInfo = std::variant<
        Info::LiveStreamPtr,
        Info::RecordDownloadPtr,
        Info::RecordPlaybackPtr,
        Info::VoiceBroadcastPtr>;

	std::unique_ptr<ExecutingInviteCmd> Clone();

	ExecutingInviteCmd()
	{}

	ExecutingInviteCmd(const ExecutingInviteCmd& rhs)
	{
		*this=rhs;
	}

	ExecutingInviteCmd& operator=(const ExecutingInviteCmd& cmd);

	std::string GetId() const
	{
		std::ostringstream oss;
		oss<<static_cast<uint8_t>(cmdType);
		return cmdSenderId+deviceId+oss.str();
	}

    std::string cmdSenderId;
    std::string deviceId;
    CmdType cmdType;
    int cmdSeq = 0;

    InviteState state;

    OperationInfo operationInfo;

    std::string localDeviceId;
    std::string localSipIp;
    std::string deviceSipIp;
    std::string deviceSipPort;
    int sipCid = 0;
    int sipDid = 0;
    int sipTidBye = 0;
    std::string sipMsgCallIdNumber;
    std::string sipMsgFromTag;
    std::string sipMsgToTag;
    std::string sipMsgCseqNumber;
} ExecutingInviteCmd;

using ExecutingInviteCmdPtr = std::unique_ptr<ExecutingInviteCmd>;


}//namespace GBGateway

#endif//__EXECUTING_INVITE_CMD_H__
