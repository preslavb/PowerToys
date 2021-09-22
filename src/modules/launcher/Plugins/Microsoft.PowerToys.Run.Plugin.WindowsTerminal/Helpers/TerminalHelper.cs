// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

namespace Microsoft.PowerToys.Run.Plugin.WindowsTerminal.Helpers
{
    public static class TerminalHelper
    {
        public static string GetArguments(string profileName, bool openNewTab)
        {
            return openNewTab ? $"--window 0 nt --profile \"{profileName}\"" : $"--profile \"{profileName}\"";
        }
    }
}
