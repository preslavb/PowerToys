// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using Microsoft.Plugin.WebSearch.WebSearchHelper;

namespace Microsoft.Plugin.WebSearch.Interfaces
{
    public interface IWebSearchParser
    {
        bool ParseQuery(string input, out WebsearchParserResult result);
    }
}
