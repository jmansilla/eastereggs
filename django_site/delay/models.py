from datetime import datetime

from django.core.exceptions import ValidationError
from django.db import models
from django.dispatch import receiver
from django.utils import timezone

from markdownfield.models import MarkdownField, RenderedMarkdownField
from markdownfield.validators import VALIDATOR_STANDARD

from .generate_password import generate_password


def validate_repo_name(value):
    this_year = timezone.now().year
    prefix = f"so{this_year}lab"
    format = f"format is {prefix}ZgXX where Z is the Lab number and XX is the actual group number (it may start with 0)"
    if not value.startswith(prefix):
        raise ValidationError(f"Repo name must start with {prefix}")
    if len(value) != 13:
        raise ValidationError(f"Repo name must be 13 characters long: {format}")
    if value[9] not in "12345":
        raise ValidationError(f"Lab number must be a number between 1 and 5: {format}")
    if not value[10] == "g" or not value[11:].isdigit():
        raise ValidationError(f"Grupo number in repo name must be a 2 digit number: format: {format}")


class SOGroup(models.Model):
    group_name = models.CharField(max_length=100, null=True, blank=True, help_text="Optional. The name of the group")
    repo_name = models.CharField(max_length=13, unique=True, validators=[validate_repo_name],
                                 help_text="The repo name must follow the pattern soYYYYlabZgXX where YYYY is current year, "
                                            "Z is the Lab number, and XX is the actual group number (it may start with 0)")

    current_delay = models.IntegerField(default=0, help_text="Current delay in milliseconds")
    delay_increment = models.IntegerField(default=25, help_text="Delay increment in milliseconds")
    delay_roof = models.IntegerField(default=10000, help_text="Maximum delay in milliseconds")
    wasted_time = models.IntegerField(default=0, editable=False, help_text="Total wasted time accumulated in milliseconds")

    password_to_win = models.CharField(max_length=100, editable=False,
                                       help_text="AutoGenerated unique key for this group. They need to use it to win the challenge")
    challenge_won = models.BooleanField(default=False, editable=False, help_text="True if the group has won the challenge")
    challenge_won_timestamp = models.DateTimeField(default=None, null=True, blank=True, editable=False,
                                            help_text="Leave Empty. It'll be filled when the group wins the challenge")

    class Meta:
        verbose_name = 'S.O.LabGroup'
        verbose_name_plural = 'S.O.LabGroups'

    def __str__(self) -> str:
        return f'S.O.LabGrup {self.repo_name} {self.group_name}'

    def _build_message_to_user(self, msg):
        return f"message-to-user: {msg}\n"

    def handle_ping(self, user_id, tampered=False):
        msg = ""
        if self.challenge_won:  # was already won. Just return delay 0 and consider it closed
            pp = PingPong.objects.create(group=self, user_id=user_id, delay_recommended=0, was_closed=True)
            msg += self._build_message_to_user(f"Congratulations. Challenge won at {self.challenge_won_timestamp}. No more delays.")
        else:
            pp = PingPong.objects.create(group=self, user_id=user_id, delay_recommended=self.current_delay, tampering_attempt=tampered)
            if self.current_delay <= self.delay_roof:
                # Yes, there may be a race condition here. I'm not dealing with it.
                self.current_delay += self.delay_increment
            self.save()
            PWD = self.password_to_win
            msg += f"(Not-so)-hidden-message: To win the challenge, append \"&password_to_win={PWD}\" to the url. "
            msg += f"The easiest way to do that, is to add the following argument to the function causing the delay:\n"
            msg += f"some_function_name(\"{PWD}\"); instead of some_function_name(NULL);\n"

            extra_msgs = ""
            for e_msg in Deadline.get_msgs_to_print_while_delaying_deadline():
                extra_msgs += self._build_message_to_user(e_msg.text.strip())
            if extra_msgs:
                msg += f"{extra_msgs}\n"

            if pp.tampering_attempt:
                msg += self._build_message_to_user("Wrong password. You have been penalized")

        return pp, msg

    def handle_win_attempt(self, user_id, password_sent_by_user):
        if password_sent_by_user != self.password_to_win:
            return self.handle_ping(user_id, tampered=True)

        # Only here if the password is correct
        if not self.challenge_won:
            self.challenge_won = True
            self.challenge_won_timestamp = datetime.now()
            self.save()
        return self.handle_ping(user_id)

    def handle_close(self, pp_id):
        try:
            pp = PingPong.objects.get(id=pp_id)
        except PingPong.DoesNotExist:
            return None

        pp.closed = True
        pp.closed_timestamp = timezone.now()
        pp.save()
        delta = pp.closed_timestamp - pp.timestamp
        pp.group.wasted_time += delta.seconds * 1000 + round(delta.microseconds / 1000)
        pp.group.save()
        return pp


@receiver(models.signals.pre_save, sender=SOGroup, weak=False)
def generate_group_password(sender, instance, **kwargs):
    if not instance.password_to_win:
        instance.password_to_win = generate_password()


class PingPong(models.Model):
    timestamp = models.DateTimeField(auto_now_add=True)
    delay_recommended = models.IntegerField(default=0)  # how much delay it's suggested the user
    group = models.ForeignKey(SOGroup, on_delete=models.CASCADE)
    user_id = models.CharField(max_length=100, help_text="User-Id of whom initiated the ping")
    closed = models.BooleanField(default=False, help_text="True if the ping was closed (ie, it awaited the delay and closed it)")
    closed_timestamp = models.DateTimeField(default=None, null=True, blank=True, help_text="Leave Empty. It'll be filled when the ping was closed")
    tampering_attempt = models.BooleanField(default=False, help_text="True if the ping was tampered with wrong key")


class Deadline(models.Model):
    SHOW_CHALLENGE_EXPLANATION = 'show_challenge_explanation'

    name = models.CharField(max_length=100, unique=True)
    deadline = models.DateTimeField()
    text = MarkdownField(rendered_field='text_rendered', default='', validator=VALIDATOR_STANDARD)
    text_rendered = RenderedMarkdownField(default='')

    def __str__(self) -> str:
        return self.name

    @classmethod
    def get_show_challenge_explanation_deadline(cls):
        return cls.objects.filter(name=cls.SHOW_CHALLENGE_EXPLANATION).first()

    @classmethod
    def get_msgs_to_print_while_delaying_deadline(cls):
        # from datetime import timedelta
        # now = timezone.now() + timedelta(days=30)  # for testing
        now = timezone.now()
        msgs = cls.objects.exclude(name=cls.SHOW_CHALLENGE_EXPLANATION).exclude(deadline__gt=now).order_by('deadline')
        return msgs
