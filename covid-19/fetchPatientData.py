"""
Script for fetch_websiteing Coronavirus Case stats from websites
- Panu Simolin
"""

from bs4 import BeautifulSoup
import urllib.request
# import json
import argparse
import sys
import re


class WorldOMeterCoronaPage(object):
    """ Contains Covid-19 update page from www.worldometers.info """

    def __init__(self, target_url):
        """ The init """
        self.url = target_url

    def fetch_website(self):
        """ Loads the page html from target url and parses it to soup. Returns true on success,
            false otherwise. The loaded page is stored in this instance. """
        try:
            request = urllib.request.Request(
                self.url,
                headers={
                    'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.162 Safari/537.36'
                }
            )
            response = urllib.request.urlopen(request)
        except (urllib.error.HTTPError, urllib.error.URLError):
            print("Site < %s > unavailable..." % self.url)
            return False

        self.soup = BeautifulSoup(response.read(), "html.parser")

        return True

    def find_graph_script(self, graph_title):
        """ Parses the page html and extracts the javascript that contains a graph matching
            the given title. """
        all_graphs = self.soup.findAll('div', {'class': 'row graph_row'})
        for graph in all_graphs:
            if graph.find_next('h3').text.startswith(graph_title):
                return graph.find_next('script', type="text/javascript").text

        return None

    def get_data_from_script(self, script):
        """ Parses the javascript and forms two arrays: one for dates and one for data. """
        dates_line = ""
        data_line = ""
        for line in script.split('\n'):
            stripped = line.strip()
            if stripped.startswith('categories'):
                dates_line = stripped
            elif line.strip().startswith('data'):
                data_line = stripped 

        dateStr = dates_line.split('[')[-1].split(']')[0]
        dates = []
        for d in dateStr.split(','):
            dates.append(d.strip('"'))

        data = []
        dataStr = data_line.split('[')[-1].split(']')[0]
        for d in dataStr.split(','):
            if d == 'null':
                data.append(0)
            else:
                data.append(int(d))

        return dates, data

    def fetch(self):
        """ Fetches the website and parses the containing data. Data is stored in the instance
            itself. Returns true if successful """
        if not self.fetch_website():
            return False

        dnc_script = self.find_graph_script('Daily New Cases')
        daily_new_dates, daily_new_data = self.get_data_from_script(dnc_script)

        tot_script = self.find_graph_script('Total Coronavirus Cases')
        total_dates, total_data = self.get_data_from_script(tot_script)

        if len(daily_new_dates) != len(total_data):
            print("The lenght of the datasets don't match")
            return False

        if daily_new_dates[0] != total_dates[0]:
            print("The data sets start from different points of time")
            return False

        if daily_new_dates[-1] != total_dates[-1]:
            print("The data sets end in different points of time")
            return False

        self.dates = total_dates
        self.total_cases = total_data
        self.daily_new_cases = daily_new_data

        return True


def configure_arg_parse():
    """Sets up the arguments that can be given to this script with help."""
    parser = argparse.ArgumentParser(description="Just testing at this point")
    parser.add_argument("-u", dest='url', type=str, help='full url')

    args = parser.parse_args()
    if args.url is None:
        print("Running the default test kit.")
        args.url = "https://www.worldometers.info/coronavirus/country/sweden/"

    return parser, args


def main():
    """ Main function for testing """
    parser, args = configure_arg_parse()

    if args is None:
        return 4

    example = WorldOMeterCoronaPage("https://www.worldometers.info/coronavirus/country/us/")
    if example.fetch():
        print("Success")
        # plot(example.total_cases, example.daily_new_cases)
    else:
        print("Oops there is something wrong in this program")
        return 7

    invalid_url = "http://asdfasdfasdf.dy.fi/"
    test_page = WorldOMeterCoronaPage(invalid_url)
    if test_page.fetch_website():
        print("FAILURE! this step should not return success")
        return 8

    test_page = WorldOMeterCoronaPage(args.url)
    if not test_page.fetch_website():
        print("FAILURE! Fetching this page should succeed")
        return 8

    if len(test_page.find_graph_script('Daily New Cases')) < 300:
        print("FAILURE! finding graph script from Sweden failed")
        return 9
    if len(test_page.find_graph_script('Total Coronavirus Death')) < 300:
        print("FAILURE! finding graph script from Sweden failed")
        return 9
    if len(test_page.find_graph_script('Total Coronavirus Cases')) < 300:
        print("FAILURE! finding graph script from Sweden failed")
        return 9

    second_url = 'https://www.worldometers.info/coronavirus/country/denmark/'
    test_page = WorldOMeterCoronaPage(second_url)
    if not test_page.fetch_website():
        print("FAILURE! Fetching this page should succeed")
        return 8

    daily_new_java_script = test_page.find_graph_script('Daily New Cases')
    dates, data = test_page.get_data_from_script(daily_new_java_script)
    if len(dates) < 10:
        print("FAILURE! Something wrong with the parsed data. Denmark")
        return 9
    if dates[3] != 'Feb 18':
        print("FAILURE! date (%s) did not match to the expected (Feb 18)" % dates[3])
        return 9
    if sum(data) < 1010:
        print("FAILURE! The sum of found cases can't be accurete")
        return 9
    if len(test_page.find_graph_script('Total Coronavirus Death')) < 300:
        print("FAILURE! finding graph script from Denmark failed")
        return 9
    if len(test_page.find_graph_script('Total Coronavirus Cases')) < 300:
        print("FAILURE! finding graph script from Denmark failed")
        return 9

    print("all tests PASSED")
    return 0
    # ToDo. Test loading page that does not load
    # ToDo. Load the coronavirus country sweden page


if __name__ == "__main__":
    sys.exit(main())
