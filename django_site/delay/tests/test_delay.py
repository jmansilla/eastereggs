from unittest.mock import patch
from time import sleep

from django.test import TestCase
from django.utils import timezone


from delay.models import SOGroup, PingPong, Deadline

# Create your tests here.

REPO_NAME = "so2024lab1g05"


class TestGroupBasic(TestCase):
    def setUp(self) -> None:
        self.group = SOGroup.objects.create(repo_name=REPO_NAME)

    def test_group_is_initially_not_won(self):
        self.assertEqual(self.group.challenge_won, False)
        self.assertIsNone(self.group.challenge_won_timestamp)


class TestHandlePing(TestCase):
    def setUp(self) -> None:
        self.group = SOGroup.objects.create(repo_name=REPO_NAME)
        self.sample_uid = "user_id"

    def test_handle_ping_returns_a_ping_and_a_message(self):
        pp, msgs = self.group.handle_ping(self.sample_uid)
        self.assertIsInstance(pp, PingPong)
        self.assertIsInstance(msgs, str)
        self.assertEqual(msgs, "")

    def test_pings_are_registered_under_the_group_and_user(self):
        pp, msgs = self.group.handle_ping(self.sample_uid)
        self.assertEqual(pp.user_id, self.sample_uid)
        self.assertEqual(pp.group, self.group)

    def test_pings_are_initially_not_closed(self):
        pp, msgs = self.group.handle_ping(self.sample_uid)
        self.assertEqual(pp.closed, False)

    def test_pings_are_initially_not_tampered(self):
        pp, msgs = self.group.handle_ping(self.sample_uid)
        self.assertEqual(pp.tampering_attempt, False)

    def test_ping_has_a_recommended_delay(self):
        initial_delay = self.group.current_delay
        pp, msgs = self.group.handle_ping(self.sample_uid)
        self.assertIsInstance(pp.delay_recommended, int)
        self.assertEqual(pp.delay_recommended, initial_delay)

    def test_each_ping_increases_the_delay(self):
        pp, msgs = self.group.handle_ping(self.sample_uid)
        pp2, msgs = self.group.handle_ping(self.sample_uid)
        self.assertGreater(pp2.delay_recommended, pp.delay_recommended)
        self.assertEqual(pp2.delay_recommended, pp.delay_recommended + self.group.delay_increment)

    def test_delay_roof_is_respected(self):
        self.group.delay_roof = 100
        self.group.current_delay = 0
        self.group.delay_increment = 60
        self.group.save()
        pp1, msgs = self.group.handle_ping(self.sample_uid)
        pp2, msgs = self.group.handle_ping(self.sample_uid)
        pp3, msgs = self.group.handle_ping(self.sample_uid)
        pp4, msgs = self.group.handle_ping(self.sample_uid)
        self.assertEqual([p.delay_recommended for p in [pp1, pp2, pp3, pp4]], [0, 60, 100, 100])

    def test_handle_ping_tampered_returns_a_ping_and_a_message(self):
        pp, msgs = self.group.handle_ping(self.sample_uid, tampered=True)
        self.assertIsInstance(pp, PingPong)
        self.assertIsInstance(msgs, str)
        self.assertIn("message-to-user: ERROR", msgs)

class TestHandleClose(TestCase):
    def setUp(self) -> None:
        self.group = SOGroup.objects.create(repo_name=REPO_NAME)
        self.sample_uid = "user_id"
        self.ping = self.group.handle_ping(self.sample_uid)[0]

    def test_handle_close_marks_as_closed_and_returns_a_ping(self):
        pp = self.group.handle_close(self.ping.id)
        self.assertIsInstance(pp, PingPong)
        self.assertTrue(pp.closed)

    def test_handle_close_sets_closed_timestamp_and_increases_wasted_time(self):
        prev_wasted_time = self.group.wasted_time
        sleep(0.1) # 100 milliseconds
        pp = self.group.handle_close(self.ping.id)
        self.assertIsNotNone(pp.closed_timestamp)
        # reload the group
        self.group = SOGroup.objects.get(id=self.group.id)
        self.assertGreater(self.group.wasted_time, prev_wasted_time)
        # given that we wait 100 milliseconds, the wasted time should have increased at least by 100 milliseconds
        self.assertGreaterEqual(self.group.wasted_time, prev_wasted_time + 100)


class TestPingWhenAlreadyWon(TestCase):
    def setUp(self) -> None:
        self.group = SOGroup.objects.create(repo_name=REPO_NAME)
        self.group.challenge_won = True
        self.group.challenge_won_timestamp = timezone.now()
        self.group.save()
        self.sample_uid = "user_id"

    def test_handle_ping_returns_delay_0_and_congratulations(self):
        pp, msgs = self.group.handle_ping(self.sample_uid)
        self.assertEqual(pp.delay_recommended, 0)
        self.assertIn('message-to-user: SUCCESS: Congratulations', msgs)
        self.assertTrue(pp.closed)


class TestHandleWinAttempt(TestCase):
    def setUp(self) -> None:
        self.group = SOGroup.objects.create(repo_name=REPO_NAME)
        self.sample_uid = "user_id"
        self.pwd_ok = self.group.password_to_win
        self.pwd_bad = 'BAD' + self.pwd_ok + 'BAD'

    def test_handle_win_attempt_with_correct_password_sets_the_group_as_won(self):
        pp, msgs = self.group.handle_win_attempt(self.sample_uid, self.pwd_ok)
        self.assertTrue(self.group.challenge_won)
        self.assertIsNotNone(self.group.challenge_won_timestamp)

    def test_handle_win_attempt_with_correct_password_calls_handle_ping(self):
        with patch.object(self.group, 'handle_ping') as mock_handle_ping:
            mock_handle_ping.return_value = ('a', 'b')
            pp, msgs = self.group.handle_win_attempt(self.sample_uid, self.pwd_ok)
            mock_handle_ping.assert_called_once_with(self.sample_uid)
            self.assertEqual((pp, msgs), mock_handle_ping.return_value)

    # Bad Win Attempts tests below
    def test_handle_win_attempt_with_incorrect_password_creates_a_tampered_ping(self):
        pp, msgs = self.group.handle_win_attempt(self.sample_uid, self.pwd_bad)
        self.assertEqual(pp.tampering_attempt, True)
        self.assertEqual(self.group.challenge_won, False)

    def test_handle_win_attempt_with_incorrect_password_calls_handle_ping(self):
        with patch.object(self.group, 'handle_ping') as mock_handle_ping:
            mock_handle_ping.return_value = ('a', 'b')
            pp, msgs = self.group.handle_win_attempt(self.sample_uid, self.pwd_bad)
            mock_handle_ping.assert_called_once_with(self.sample_uid, tampered=True)
            self.assertEqual((pp, msgs), mock_handle_ping.return_value)

    def test_after_win_any_other_bad_win_attempt_still_leaves_the_group_as_won(self):
        pp1, msgs = self.group.handle_win_attempt(self.sample_uid, self.pwd_ok)
        self.assertTrue(self.group.challenge_won)
        pp2, msgs = self.group.handle_win_attempt(self.sample_uid, self.pwd_bad)
        self.assertTrue(self.group.challenge_won)
