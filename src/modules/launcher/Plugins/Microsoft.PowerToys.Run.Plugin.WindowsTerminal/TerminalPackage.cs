// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;

namespace Microsoft.PowerToys.Run.Plugin.WindowsTerminal
{
    public class TerminalPackage
    {
        public string AppUserModelId { get; }

        public Version Version { get; }

        public string DisplayName { get; }

        public string SettingsPath { get; }

        public TerminalPackage(string appUserModelId, Version version, string displayName, string settingsPath)
        {
            AppUserModelId = appUserModelId;
            Version = version;
            DisplayName = displayName;
            SettingsPath = settingsPath;
        }
    }
}
