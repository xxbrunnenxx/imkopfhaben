import type { PortalDom } from './dom';

interface PortalUiStateControllers {
  localAiController: {
    getLocalAiBaseUrl: () => string;
    isLocalAiBusy: () => boolean;
  };
  timeController: {
    isClockBusy: () => boolean;
    isLocationBusy: () => boolean;
    isTimezoneBusy: () => boolean;
  };
  wifiController: {
    getSelectedNetwork: () => string;
    isCheckingStatus: () => boolean;
    isConnecting: () => boolean;
    isCurrentlyConnected: () => boolean;
    isScanning: () => boolean;
    isSelectedConnected: () => boolean;
    requiresPassword: () => boolean;
  };
}

interface UpdatePortalUiStateDeps {
  controllers: PortalUiStateControllers;
  dom: PortalDom;
}

export function updatePortalUiState(deps: UpdatePortalUiStateDeps) {
  const { controllers, dom } = deps;

  // --- WiFi ---
  const wifiBusy =
    controllers.wifiController.isScanning() ||
    controllers.wifiController.isConnecting() ||
    controllers.wifiController.isCheckingStatus();
  const isSelectedConnected = controllers.wifiController.isSelectedConnected();
  const selectedNetwork = controllers.wifiController.getSelectedNetwork().trim();
  const passwordRequired =
    selectedNetwork.length > 0 &&
    !isSelectedConnected &&
    controllers.wifiController.requiresPassword();
  const hasWifiActionTarget =
    isSelectedConnected ||
    (selectedNetwork.length > 0 &&
      (!passwordRequired || dom.passwordInput.value.trim().length > 0));

  dom.scanBtn.disabled = wifiBusy;
  dom.connectBtn.disabled = wifiBusy || !hasWifiActionTarget;
  dom.wifiStatusCard.actionDisabled = wifiBusy;
  dom.passwordInput.disabled = controllers.wifiController.isConnecting();
  if (controllers.wifiController.isConnecting()) {
    dom.connectBtn.textContent = 'Connecting...';
    dom.wifiStatusCard.actionLabel = 'Connecting...';
  } else if (isSelectedConnected) {
    dom.connectBtn.textContent = 'Disconnect';
    dom.wifiStatusCard.actionLabel = 'Disconnect';
  } else {
    dom.connectBtn.textContent = 'Connect';
    dom.wifiStatusCard.actionLabel = controllers.wifiController.isCurrentlyConnected()
      ? 'Disconnect'
      : 'Connect';
  }

  // --- Time / timezone ---
  const timeConfigBusy =
    controllers.timeController.isTimezoneBusy() ||
    controllers.timeController.isLocationBusy() ||
    controllers.timeController.isClockBusy();
  dom.timezoneSelect.disabled = timeConfigBusy;
  dom.manualDateInput.disabled = timeConfigBusy;
  dom.manualTimeInput.disabled = timeConfigBusy;
  dom.timezoneLocationSaveBtn.disabled =
    timeConfigBusy || dom.timezoneSelect.value.trim().length === 0;
  dom.timezoneLocationClearBtn.disabled =
    timeConfigBusy || dom.timezoneSelect.value.trim().length === 0;

  // --- Local AI server (always available on Followup) ---
  // No secret to mask -- the field stays editable at all times, gated only by "busy" and
  // "is there text to save", not by a has-key flag the way the old Gemini card was.
  const localAiBusy = controllers.localAiController.isLocalAiBusy();
  dom.localAiBaseUrlInput.readOnly = localAiBusy;
  dom.localAiBaseUrlInput.disabled = localAiBusy;
  dom.localAiSaveBtn.disabled = localAiBusy || dom.localAiBaseUrlInput.value.trim().length === 0;
  dom.localAiResetBtn.disabled = localAiBusy;
}
