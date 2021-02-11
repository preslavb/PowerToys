// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Globalization;
using Microsoft.Plugin.WebSearch.Interfaces;

namespace Microsoft.Plugin.WebSearch.WebSearchHelper
{
    public class WebSearchParser : IWebSearchParser
    {
        private static bool QueryStartsWith(string input, string start, out string query)
        {
            if (input.StartsWith(start, StringComparison.CurrentCultureIgnoreCase))
            {
                query = input.Substring(start.Length);
                return true;
            }

            query = null;
            return false;
        }

        private static bool ParseWebsearchQuery(string input, string keyword, string name, string url, out WebsearchParserResult result)
        {
            if (QueryStartsWith(input, keyword, out string query))
            {
                var queryUrl = string.Format(CultureInfo.InvariantCulture, url, query);
                var builder = new UriBuilder(queryUrl);

                result = new WebsearchParserResult
                {
                    Url = builder.Uri,
                    Name = name,
                    Query = query,
                };

                return true;
            }

            result = null;
            return false;
        }

        public bool ParseQuery(string input, out WebsearchParserResult result)
        {
            if (input == null)
            {
                result = null;
                return false;
            }

            if (ParseWebsearchQuery(input, "g ", "Google", "http://www.google.com/search?q={0}", out result))
            {
                return true;
            }

            if (ParseWebsearchQuery(input, "gh ", "Github", "https://github.com/search?q={0}", out result))
            {
                return true;
            }

            if (ParseWebsearchQuery(input, "b ", "Bing", "https://www.bing.com/search?q={0}", out result))
            {
                return true;
            }

            if (ParseWebsearchQuery(input, "y ", "YouTube", "https://www.youtube.com/results?search_query={0}", out result))
            {
                return true;
            }

            if (ParseWebsearchQuery(input, "b ", "Bing", "https://www.bing.com/search?q={0}", out result))
            {
                return true;
            }

            if (ParseWebsearchQuery(input, "d ", "Dict", "https://www.dict.cc/?s={0}", out result))
            {
                return true;
            }

            result = null;
            return false;
        }
    }
}
