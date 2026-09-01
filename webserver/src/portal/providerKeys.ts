import type {
  LocalAiModuleResponse,
  LocalAiModuleSettings,
  ModuleStatusResponse,
  OpenAiModuleResponse,
  OpenAiModuleSettings,
  StatusType,
  ValidatableField,
} from './types';

type ProviderInput = ValidatableField & { readOnly: boolean };
type ProviderSettings = { has_key?: boolean; last4?: string };
type ProviderResponse = {
  message?: string;
  settings?: ProviderSettings;
};

interface ProviderState {
  hasKey: boolean;
  isBusy: boolean;
  last4: string;
  resetApi: string;
  settingsApi: string;
}

// The local AI server has no secret to mask -- the settings/reset routes echo the same
// GET/PATCH/reset/runtime shape as the old Gemini API-key module, but the field is a plain,
// always-visible base URL rather than a write-only masked key. Kept as its own small controller
// instead of forcing it through applyProviderSettings/saveProviderKey/clearProviderKey below,
// which assume a secret that gets masked as "******last4" -- that assumption doesn't hold here.
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
  fetchOpenAiModuleJson: (
    path: string,
    init?: RequestInit
  ) => Promise<OpenAiModuleResponse>;
  localAiBaseUrlInput: ProviderInput;
  isLocalAiModuleActive: () => boolean;
  isOpenAiModuleActive: () => boolean;
  notifyLocalAi: (message: string, type?: StatusType) => void;
  notifyOpenAi: (message: string, type?: StatusType) => void;
  openAiApiKeyInput: ProviderInput;
  updateButtons: () => void;
}

function applyProviderSettings(
  state: ProviderState,
  input: ProviderInput,
  settings?: ProviderSettings
) {
  state.hasKey = settings?.has_key === true;
  state.last4 = typeof settings?.last4 === 'string' ? settings.last4 : '';
  input.value = state.hasKey
    ? state.last4
      ? `******${state.last4}`
      : '******'
    : '';
}

async function saveProviderKey(
  state: ProviderState,
  input: ProviderInput,
  notify: (message: string, type?: StatusType) => void,
  updateButtons: () => void,
  fetchJson: (path: string, init?: RequestInit) => Promise<ProviderResponse>,
  progressMessage: string,
  successMessage: string
) {
  const apiKey = input.value.trim();
  if (!apiKey) {
    notify(`${successMessage} is required.`, 'warning');
    return;
  }

  state.isBusy = true;
  notify(progressMessage, 'info');
  updateButtons();

  try {
    const data = await fetchJson(state.settingsApi, {
      method: 'PATCH',
      body: JSON.stringify({ api_key: apiKey }),
    });
    applyProviderSettings(state, input, data.settings);
    notify(data.message || `${successMessage} stored.`, 'success');
  } catch (error) {
    console.error(`${successMessage} save failed:`, error);
    notify(
      error instanceof Error ? error.message : `Failed to store ${successMessage}.`,
      'error'
    );
  } finally {
    state.isBusy = false;
    updateButtons();
  }
}

async function clearProviderKey(
  state: ProviderState,
  input: ProviderInput,
  notify: (message: string, type?: StatusType) => void,
  updateButtons: () => void,
  fetchJson: (path: string, init?: RequestInit) => Promise<ProviderResponse>,
  progressMessage: string,
  successMessage: string
) {
  state.isBusy = true;
  notify(progressMessage, 'info');
  updateButtons();

  try {
    const data = await fetchJson(state.resetApi, {
      method: 'POST',
    });
    applyProviderSettings(state, input, data.settings);
    notify(data.message || successMessage, 'success');
  } catch (error) {
    console.error(`${successMessage} failed:`, error);
    notify(
      error instanceof Error ? error.message : `Failed to clear ${successMessage}.`,
      'error'
    );
  } finally {
    state.isBusy = false;
    updateButtons();
  }
}

export function createProviderKeysController(deps: ProviderKeysDeps) {
  const localAiState: LocalAiState = {
    isBusy: false,
    baseUrl: '',
    resetApi: '/api/settings/local_ai/reset',
    settingsApi: '/api/settings/local_ai',
  };

  const openAiState: ProviderState = {
    hasKey: false,
    isBusy: false,
    last4: '',
    resetApi: '/api/settings/openai/reset',
    settingsApi: '/api/settings/openai',
  };

  function updateLocalAiRoutes(module?: ModuleStatusResponse) {
    const routes = module?.routes;
    localAiState.settingsApi = routes?.settings || '/api/settings/local_ai';
    localAiState.resetApi = routes?.reset || '/api/settings/local_ai/reset';
  }

  function updateOpenAiRoutes(module?: ModuleStatusResponse) {
    const routes = module?.routes;
    openAiState.settingsApi = routes?.settings || '/api/settings/openai';
    openAiState.resetApi = routes?.reset || '/api/settings/openai/reset';
  }

  function applyLocalAiSettings(settings?: LocalAiModuleSettings) {
    localAiState.baseUrl = typeof settings?.base_url === 'string' ? settings.base_url : '';
    deps.localAiBaseUrlInput.value = localAiState.baseUrl;
  }

  function applyOpenAiSettings(settings?: OpenAiModuleSettings) {
    applyProviderSettings(openAiState, deps.openAiApiKeyInput, settings);
  }

  function clearOpenAiSettings() {
    openAiState.hasKey = false;
    openAiState.last4 = '';
    deps.openAiApiKeyInput.value = '';
    deps.notifyOpenAi('');
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

  async function saveOpenAiKey() {
    if (!deps.isOpenAiModuleActive() || openAiState.isBusy || openAiState.hasKey) {
      return;
    }

    const apiKey = deps.openAiApiKeyInput.value.trim();
    if (!apiKey) {
      deps.notifyOpenAi('OpenAI API key is required.', 'warning');
      return;
    }

    await saveProviderKey(
      openAiState,
      deps.openAiApiKeyInput,
      deps.notifyOpenAi,
      deps.updateButtons,
      deps.fetchOpenAiModuleJson,
      'Saving OpenAI API key...',
      'OpenAI API key'
    );
  }

  async function clearOpenAiKey() {
    if (!deps.isOpenAiModuleActive() || openAiState.isBusy) {
      return;
    }

    await clearProviderKey(
      openAiState,
      deps.openAiApiKeyInput,
      deps.notifyOpenAi,
      deps.updateButtons,
      deps.fetchOpenAiModuleJson,
      'Clearing OpenAI API key...',
      'OpenAI API key cleared.'
    );
  }

  return {
    applyLocalAiSettings,
    applyOpenAiSettings,
    clearOpenAiKey,
    clearOpenAiSettings,
    getLocalAiBaseUrl: () => localAiState.baseUrl,
    getOpenAiHasKey: () => openAiState.hasKey,
    isLocalAiBusy: () => localAiState.isBusy,
    isOpenAiBusy: () => openAiState.isBusy,
    resetLocalAiBaseUrl,
    saveLocalAiBaseUrl,
    saveOpenAiKey,
    updateLocalAiRoutes,
    updateOpenAiRoutes,
  };
}
