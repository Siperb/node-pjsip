const PJ_SIP = require('./lib');

let session = {
  Id: 0,                    // This Session ID is in sync with webview Session ID
  Direction: 'inbound',
  State: 'active',
  PjSipSession:null
}
let Sessions = []; 

// Methods To Handle PJ SIP
PJ_SIP.Init();

// PJ_SIP.RegisterAccount({
//   aor: "sip:yAS43lAg8L@innovateasterisk.com",
//   registrar: 'sip:yAS43lAg8L@172.31.28.116:5060',
//   username: 'yAS43lAg8L',
//   password: '123456',
//   protocol: 'UDP'
// });

// let PjSipSession = PJ_SIP.Invite({
//   to: 'sip:conrad@innovateasterisk.com',
//   withVideo: false,
// });

// PJ_SIP.SetActiveSession(PjSipSession);

// PJ_SIP.CancelInvite(PjSipSession);
// PJ_SIP.DeclineInvite(PjSipSession);
// PJ_SIP.EndSession(PjSipSession);

/// PJ_SIP.Mute(PjSipSession);
// PJ_SIP.Unmute(PjSipSession);
// PJ_SIP.Hold(PjSipSession);
// PJ_SIP.Unhold(PjSipSession);

// PJ_SIP.Shutdown();

// Events to respond to PJ SIP events
// PJ_SIP.on('Registered', (data) => {
//   console.log('Registered:', data);
// });
// PJ_SIP.on('RegisterFailed', (data) => {
//   console.log('Register Failed:', data);
// });
// PJ_SIP.on('Unregistered', (data) => {
//   console.log('Unregistered:', data);
// });

// PJ_SIP.on('IncomingInvite', (data) => {
//   console.log('Incoming Invite:', data);
// });
// PJ_SIP.on('SessionStateChange', (data) => {
//   console.log('Session State:', data);
// });
// PJ_SIP.on('Message', (data) => {
//   console.log('Message:', data);
// });