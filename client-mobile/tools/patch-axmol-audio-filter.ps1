# Adds a per-source low-pass EQ API to the vendored axmol AudioEngine.
#
# The web client routes sounds through biquad EQ nodes ("muffled", "club") and a
# reverb convolver. axmol exposes EAX reverb but no per-source EQ, so this patch
# adds `AudioEngine::setFilterProperties(audioId, enabled, gain, gainHF)` backed
# by OpenAL EFX `AL_FILTER_LOWPASS` (already available through
# AudioEffectsExtension). It is idempotent and safe to run after every
# tools/vendor-axmol.ps1 copy.
param(
    [string]$AxmolRoot = (Join-Path $PSScriptRoot '..\axmol')
)
$ErrorActionPreference = 'Stop'

$audioDir = Join-Path $AxmolRoot 'core\audio'
if (-not (Test-Path (Join-Path $audioDir 'AudioEngine.h'))) {
    throw "Not an axmol tree: $AxmolRoot"
}

function Read-Text([string]$path) {
    return [IO.File]::ReadAllText($path)
}
function Write-Text([string]$path, [string]$text) {
    [IO.File]::WriteAllText($path, $text, (New-Object System.Text.UTF8Encoding($false)))
}
function Add-After([string]$text, [string]$anchor, [string]$addition, [string]$newline) {
    if (-not $text.Contains($anchor)) {
        throw "axmol patch anchor not found: $($anchor.Substring(0, [Math]::Min(60, $anchor.Length)))"
    }
    return $text.Replace($anchor, $anchor + $addition)
}

# ---------------------------------------------------------------------------
# AudioEngine.h: public API
# ---------------------------------------------------------------------------
$p = Join-Path $audioDir 'AudioEngine.h'
$t = Read-Text $p
if (-not $t.Contains('setFilterProperties')) {
    $nl = if ($t.Contains("`r`n")) { "`r`n" } else { "`n" }
    $anchor = '    static void setReverbProperties(AUDIO_ID audioId, const ReverbProperties* reverbProperties);'
    $addition = (@(
        '',
        '    /**',
        '     * Sets a per-source low-pass EQ filter. `gain` scales the whole signal and',
        '     * `gainHF` scales the high-frequency band (0..1). Passing enabled=false',
        '     * removes the filter. Implemented with EFX AL_FILTER_LOWPASS when the',
        '     * device supports ALC_EXT_EFX; a no-op otherwise.',
        '     */',
        '    static void setFilterProperties(AUDIO_ID audioId, bool enabled, float gain, float gainHF);'
    ) -join $nl)
    Write-Text $p (Add-After $t $anchor $addition $nl)
    Write-Host 'patched axmol: AudioEngine.h (setFilterProperties)'
}

# ---------------------------------------------------------------------------
# AudioEngine.cpp: static forwarding
# ---------------------------------------------------------------------------
$p = Join-Path $audioDir 'AudioEngine.cpp'
$t = Read-Text $p
if (-not $t.Contains('AudioEngine::setFilterProperties')) {
    $nl = if ($t.Contains("`r`n")) { "`r`n" } else { "`n" }
    $anchor = (@(
        'void AudioEngine::setReverbProperties(int audioId, const ReverbProperties* reverbProperties)',
        '{',
        '    if (!_audioEngineImpl)',
        '    {',
        '        return;',
        '    }',
        '',
        '    return _audioEngineImpl->setReverbProperties(audioId, reverbProperties);',
        '}'
    ) -join $nl)
    $addition = (@(
        '',
        'void AudioEngine::setFilterProperties(int audioId, bool enabled, float gain, float gainHF)',
        '{',
        '    if (!_audioEngineImpl)',
        '    {',
        '        return;',
        '    }',
        '',
        '    _audioEngineImpl->setFilterProperties(audioId, enabled, gain, gainHF);',
        '}'
    ) -join $nl)
    Write-Text $p (Add-After $t $anchor $addition $nl)
    Write-Host 'patched axmol: AudioEngine.cpp'
}

# ---------------------------------------------------------------------------
# AudioEngineImpl.h: declaration
# ---------------------------------------------------------------------------
$p = Join-Path $audioDir 'AudioEngineImpl.h'
$t = Read-Text $p
if (-not $t.Contains('setFilterProperties')) {
    $nl = if ($t.Contains("`r`n")) { "`r`n" } else { "`n" }
    $anchor = '    void setReverbProperties(AUDIO_ID audioId, const ReverbProperties* reverbProperties);'
    $addition = $nl + '    void setFilterProperties(AUDIO_ID audioId, bool enabled, float gain, float gainHF);'
    Write-Text $p (Add-After $t $anchor $addition $nl)
    Write-Host 'patched axmol: AudioEngineImpl.h'
}

# ---------------------------------------------------------------------------
# AudioEngineImpl.cpp: lookup the player and forward
# ---------------------------------------------------------------------------
$p = Join-Path $audioDir 'AudioEngineImpl.cpp'
$t = Read-Text $p
if (-not $t.Contains('AudioEngineImpl::setFilterProperties')) {
    $nl = if ($t.Contains("`r`n")) { "`r`n" } else { "`n" }
    $anchor = (@(
        'void AudioEngineImpl::setReverbProperties(AUDIO_ID audioId, const ReverbProperties* reverbProperties)',
        '{',
        '    std::unique_lock<std::recursive_mutex> lck(_threadMutex);',
        '    auto iter = _audioPlayers.find(audioId);',
        '    if (iter == _audioPlayers.end())',
        '        return;',
        '',
        '    auto player = iter->second;',
        '    lck.unlock();',
        '',
        '    player->setReverbProperties(reverbProperties);',
        '}'
    ) -join $nl)
    $addition = (@(
        '',
        'void AudioEngineImpl::setFilterProperties(AUDIO_ID audioId, bool enabled, float gain, float gainHF)',
        '{',
        '    std::unique_lock<std::recursive_mutex> lck(_threadMutex);',
        '    auto iter = _audioPlayers.find(audioId);',
        '    if (iter == _audioPlayers.end())',
        '        return;',
        '',
        '    auto player = iter->second;',
        '    lck.unlock();',
        '',
        '    player->setFilterProperties(enabled, gain, gainHF);',
        '}'
    ) -join $nl)
    Write-Text $p (Add-After $t $anchor $addition $nl)
    Write-Host 'patched axmol: AudioEngineImpl.cpp'
}

# ---------------------------------------------------------------------------
# AudioPlayer.h: declaration + filter handle
# ---------------------------------------------------------------------------
$p = Join-Path $audioDir 'AudioPlayer.h'
$t = Read-Text $p
if (-not $t.Contains('setFilterProperties')) {
    $nl = if ($t.Contains("`r`n")) { "`r`n" } else { "`n" }
    $anchor = '    void setReverbProperties(const ReverbProperties* reverbProperties);'
    $addition = (@(
        '',
        '    // Per-source low-pass EQ (EFX AL_FILTER_LOWPASS). `gain` scales the',
        '    // whole signal, `gainHF` scales the high-frequency band (0..1).',
        '    void setFilterProperties(bool enabled, float gain, float gainHF);'
    ) -join $nl)
    $t = Add-After $t $anchor $addition $nl
    $t = Add-After $t '    uint32_t _reverbEffect{};' ($nl + '    uint32_t _filterId{};') $nl
    Write-Text $p $t
    Write-Host 'patched axmol: AudioPlayer.h'
}

# ---------------------------------------------------------------------------
# AudioPlayer.cpp: EFX low-pass lifetime + setter
# ---------------------------------------------------------------------------
$p = Join-Path $audioDir 'AudioPlayer.cpp'
$t = Read-Text $p
if (-not $t.Contains('AudioPlayer::setFilterProperties')) {
    $nl = if ($t.Contains("`r`n")) { "`r`n" } else { "`n" }
    $oldClear = (@(
        'void AudioPlayer::clearEffects()',
        '{',
        '#if AX_USE_ALSOFT',
        '    if (_reverbEffect == 0)',
        '        return;',
        '',
        '    auto&& efx = AudioEffectsExtension::getInstance();',
        '',
        '    if (!efx->isAvailable())',
        '        return;',
        '',
        '    efx->bindSourceToAuxiliarySlot(_alSource, 0, 0, 0);',
        '    alSourcei(_alSource, AL_DIRECT_FILTER, 0);  // unset filter',
        '    CHECK_AL_ERROR_DEBUG();',
        '',
        '    efx->deleteAuxiliaryEffectSlot(_reverbSlot);',
        '    _reverbSlot = 0;',
        '',
        '    efx->deleteEffect(_reverbEffect);',
        '    _reverbEffect = 0;',
        '#endif',
        '}'
    ) -join $nl)
    $newClear = (@(
        'void AudioPlayer::clearEffects()',
        '{',
        '#if AX_USE_ALSOFT',
        '    auto&& efx = AudioEffectsExtension::getInstance();',
        '',
        '    if (!efx->isAvailable())',
        '        return;',
        '',
        '    if (_filterId != 0)',
        '    {',
        '        efx->bindFilterToSource(_alSource, AL_FILTER_NULL);',
        '        efx->deleteFilter(_filterId);',
        '        _filterId = 0;',
        '    }',
        '',
        '    if (_reverbEffect == 0)',
        '        return;',
        '',
        '    efx->bindSourceToAuxiliarySlot(_alSource, 0, 0, 0);',
        '    alSourcei(_alSource, AL_DIRECT_FILTER, 0);  // unset filter',
        '    CHECK_AL_ERROR_DEBUG();',
        '',
        '    efx->deleteAuxiliaryEffectSlot(_reverbSlot);',
        '    _reverbSlot = 0;',
        '',
        '    efx->deleteEffect(_reverbEffect);',
        '    _reverbEffect = 0;',
        '#endif',
        '}',
        '',
        'void AudioPlayer::setFilterProperties(bool enabled, float gain, float gainHF)',
        '{',
        '#if AX_USE_ALSOFT',
        '    auto&& efx = AudioEffectsExtension::getInstance();',
        '    if (!efx->isAvailable())',
        '        return;',
        '',
        '    if (!enabled)',
        '    {',
        '        if (_filterId != 0)',
        '        {',
        '            efx->bindFilterToSource(_alSource, AL_FILTER_NULL);',
        '            efx->deleteFilter(_filterId);',
        '            _filterId = 0;',
        '        }',
        '        return;',
        '    }',
        '',
        '    if (_filterId == 0)',
        '        _filterId = efx->genFilter();',
        '',
        '    efx->setFilterParamInt(_filterId, AL_FILTER_TYPE, AL_FILTER_LOWPASS);',
        '    efx->setFilterParamFloat(_filterId, AL_LOWPASS_GAIN, gain);',
        '    efx->setFilterParamFloat(_filterId, AL_LOWPASS_GAINHF, gainHF);',
        '    efx->bindFilterToSource(_alSource, _filterId);',
        '#else',
        '    (void)enabled;',
        '    (void)gain;',
        '    (void)gainHF;',
        '#endif',
        '}'
    ) -join $nl)
    if (-not $t.Contains($oldClear)) {
        throw 'axmol patch anchor not found: AudioPlayer::clearEffects'
    }
    Write-Text $p $t.Replace($oldClear, $newClear)
    Write-Host 'patched axmol: AudioPlayer.cpp (EFX low-pass)'
}

Write-Host 'axmol audio filter patch complete'
