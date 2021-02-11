// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;

namespace Microsoft.Plugin.WebSearch.WebSearchHelper
{
    public class WebsearchParserResult
    {
        public Uri Url { get; internal set; }

        public string Name { get; internal set; }

        public string Query { get; internal set; }
    }
}
