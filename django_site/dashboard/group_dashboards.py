from dashboards.dashboard import Dashboard
from dashboards.component import Text, Chart, Table
from dashboards.registry import registry

from dashboard.data import DashboardData, GroupTableSerializer


class MainDashboard(Dashboard):
    welcome = Text(value="This is a very ugly dashboard but it's all I can do in two hours!", grid_css_classes="span-12")
    groups_by_challenge_won = Chart(value=DashboardData.groups_by_challenge_won, grid_css_classes="span-6")
    content_types = Table(value=GroupTableSerializer)

    class Meta:
        name = "Main Dashboard"


registry.register(MainDashboard)
