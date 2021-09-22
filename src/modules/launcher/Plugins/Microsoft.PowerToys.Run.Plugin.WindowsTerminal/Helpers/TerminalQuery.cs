// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Security.Principal;
using System.Text.Json;
using Windows.Management.Deployment;

namespace Microsoft.PowerToys.Run.Plugin.WindowsTerminal.Helpers
{
    public class TerminalQuery : ITerminalQuery
    {
        private static ReadOnlyCollection<string> Packages => new List<string>
        {
            "Microsoft.WindowsTerminal",
            "Microsoft.WindowsTerminalPreview",
        }.AsReadOnly();

        private static IEnumerable<TerminalPackage> _terminals;

        public TerminalQuery()
        {
            var user = WindowsIdentity.GetCurrent().User;
            var packages = new PackageManager().FindPackagesForUser(user.Value).Where(a => Packages.Contains(a.Id.Name));

            var localAppDataPath = Environment.GetEnvironmentVariable("LOCALAPPDATA");

            _terminals = packages.Select(p =>
            {
                var aumid = p.GetAppListEntries().Single().AppUserModelId;
                var version = new Version(p.Id.Version.Major, p.Id.Version.Minor, p.Id.Version.Build, p.Id.Version.Revision);
                var settingsPath = Path.Combine(localAppDataPath, "Packages", p.Id.FamilyName, "LocalState", "settings.json");
                return new TerminalPackage(aumid, version, p.DisplayName, settingsPath);
            });
        }

        public IEnumerable<TerminalProfile> GetProfiles()
        {
            var profiles = new List<TerminalProfile>();

            foreach (var terminal in _terminals)
            {
                if (!File.Exists(terminal.SettingsPath))
                {
                    continue;
                }

                var jsonText = File.ReadAllText(terminal.SettingsPath);

                var options = new JsonDocumentOptions
                {
                    CommentHandling = JsonCommentHandling.Skip,
                };

                var json = JsonDocument.Parse(jsonText, options);

                json.RootElement.TryGetProperty("profiles", out JsonElement profilesElement);
                if (profilesElement.ValueKind != JsonValueKind.Object)
                {
                    continue;
                }

                profilesElement.TryGetProperty("list", out JsonElement profilesList);
                if (profilesList.ValueKind != JsonValueKind.Array)
                {
                    continue;
                }

                foreach (var profile in profilesList.EnumerateArray())
                {
                    profiles.Add(ParseProfile(terminal, profile));
                }
            }

            return profiles.OrderBy(p => p.Name);
        }

        private static TerminalProfile ParseProfile(TerminalPackage terminal, JsonElement profileElement)
        {
            profileElement.TryGetProperty("name", out JsonElement nameElement);
            var name = nameElement.ValueKind == JsonValueKind.String ? nameElement.GetString() : null;

            profileElement.TryGetProperty("hidden", out JsonElement hiddenElement);
            var hidden = (hiddenElement.ValueKind == JsonValueKind.False || hiddenElement.ValueKind == JsonValueKind.True) && hiddenElement.GetBoolean();

            profileElement.TryGetProperty("guid", out JsonElement guidElement);
            var guid = guidElement.ValueKind == JsonValueKind.String ? Guid.Parse(guidElement.GetString()) : null as Guid?;

            profileElement.TryGetProperty("icon", out JsonElement iconElement);
            var icon = iconElement.ValueKind == JsonValueKind.String ? iconElement.GetString() : null;

            return new TerminalProfile(terminal, name, guid, icon, hidden);
        }
    }
}
