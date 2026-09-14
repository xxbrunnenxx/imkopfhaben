import type {
  LocalAiModuleResponse,
  LocalAiModuleSettings,
  ModuleStatusResponse,
  StatusType,
  ValidatableField,
} from './types';

type ProviderInput = ValidatableField & { readOnly: boolean };

interface LocalAiState {
  isBusy: boolean;
  baseUrl: string;
  resetApi: string;
  settingsApi: string;
}

interface ProviderKeysDeps {
  fetchLocalAiModuleJson: (
    path: string,
    init?: RequestInit
  ) => Promise<LocalAiModuleResponse>;
  localAiBaseUrlInput: ProviderInput;
  isLocalAiModuleActive: () => boolean;
  notifyLocalAi: (message: string, type?: StatusType) => void;
  updateButtons: () => void;
}

export function createProviderKeysController(deps: ProviderKeysDeps) {
  const localAiState: LocalAiState = {
    isBusy: false,
    baseUrl: '',
    resetApi: '/api/settings/local_ai/reset',
    settingsApi: '/api/settings/local_ai',
  };

  function updateLocalAiRoutes(module?: ModuleStatusResponse) {
    const routes = module?.routes;
    localAiState.settingsApi = routes?.settings || '/api/settings/local_ai';
    localAiState.resetApi = routes?.reset || '/api/settings/local_ai/reset';
  }

  function applyLocalAiSettings(settings?: LocalAiModuleSettings) {
    localAiState.baseUrl = typeof settings?.base_url === 'string' ? settings.base_url : '';
    deps.localAiBaseUrlInput.value = localAiState.baseUrl;
  }

  async function saveLocalAiBaseUrl() {
    if (!deps.isLocalAiModuleActive() || localAiState.isBusy) {
      return;
    }

    const baseUrl = deps.localAiBaseUrlInput.value.trim();
    if (!baseUrl) {
      deps.notifyLocalAi('Local AI server URL is required.', 'warning');
      return;
    }

    localAiState.isBusy = true;
    deps.notifyLocalAi('Saving local AI server URL...', 'info');
    deps.updateButtons();

    try {
      const data = await deps.fetchLocalAiModuleJson(localAiState.settingsApi, {
        method: 'PATCH',
        body: JSON.stringify({ base_url: baseUrl }),
      });
      applyLocalAiSettings(data.settings);
      deps.notifyLocalAi(data.message || 'Local AI server URL stored.', 'success');
    } catch (error) {
      console.error('Local AI server URL save failed:', error);
      deps.notifyLocalAi(
        error instanceof Error ? error.message : 'Failed to store local AI server URL.',
        'error'
      );
    } finally {
      localAiState.isBusy = false;
      deps.updateButtons();
    }
  }

  async function resetLocalAiBaseUrl() {
    if (!deps.isLocalAiModuleActive() || localAiState.isBusy) {
      return;
    }

    localAiState.isBusy = true;
    deps.notifyLocalAi('Resetting local AI server URL...', 'info');
    deps.updateButtons();

    try {
      const data = await deps.fetchLocalAiModuleJson(localAiState.resetApi, {
        method: 'POST',
      });
      applyLocalAiSettings(data.settings);
      deps.notifyLocalAi(data.message || 'Local AI server URL reset to built-in default.', 'success');
    } catch (error) {
      console.error('Local AI server URL reset failed:', error);
      deps.notifyLocalAi(
        error instanceof Error ? error.message : 'Failed to reset local AI server URL.',
        'error'
      );
    } finally {
      localAiState.isBusy = false;
      deps.updateButtons();
    }
  }

  return {
    applyLocalAiSettings,
    getLocalAiBaseUrl: () => localAiState.baseUrl,
    isLocalAiBusy: () => localAiState.isBusy,
    resetLocalAiBaseUrl,
    saveLocalAiBaseUrl,
    updateLocalAiRoutes,
  };
}
